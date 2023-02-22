////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/application/application.h>
#include <ork/dataflow/dataflow.h>
#include <ork/dataflow/scheduler.h>
#include <ork/reflect/properties/register.h>
#include <ork/kernel/orklut.hpp>
#include <ork/reflect/properties/AccessorTyped.hpp>
#include <ork/reflect/properties/DirectTypedMap.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace ork::dataflow {
///////////////////////////////////////////////////////////////////////////////

DgSorter::DgSorter(const GraphData* pg, dgcontext_ptr_t ctx)
    : _dgcontext(ctx)
    , _graphdata(pg)
    , _serial(NOSERIAL) {

  _logchannel = logger()->createChannel("dgsorter-std", fvec3(0.8, 0.8, 0.4), false);
  _logchannel_reg = logger()->createChannel("dgsorter-reg", fvec3(0.4, 0.9, 0.2), false);

  /////////////////////////////////////////
  // add all modules
  /////////////////////////////////////////

  size_t inummodules = pg->numModules();
  for (size_t im = 0; im < inummodules; im++) {
    dgmoduledata_ptr_t pmod = pg->module(im);
    addModule(pmod);
  }

  /////////////////////////////////////////
  // compute depths iteratively
  /////////////////////////////////////////

  _logchannel->log("compute depths: ");

  int inumchg = -1;
  while (inumchg != 0) {
    inumchg = 0;
    for (size_t im = 0; im < inummodules; im++) {
      dgmoduledata_ptr_t pmod = pg->module(im);

      auto& node_info = _nodeinfomap[pmod];

      int inumouts = pmod->numOutputs();
      for (int op = 0; op < inumouts; op++) {
        auto poutplug  = pmod->output(op);
        size_t inumcon = poutplug->numConnections();
        int ilo        = 0;
        for (size_t ic = 0; ic < inumcon; ic++) {
          auto pin  = poutplug->connected(ic);
          auto pcon = typedModuleData<DgModuleData>(pin->_parent_module);
          int itd   = node_info._depth - 1;
          if (itd < ilo)
            ilo = itd;
        }
        if (node_info._depth > ilo && ilo != 0) {
          node_info._depth = s8(ilo); // TODO: whats the s8 for again? - its important
          inumchg++;
        }
      }
      _logchannel->log(" mod<%s> comp_depth<%d>", pmod->_name.c_str(), node_info._depth);
    }
  }

  _logchannel->log("//////////////////////");
}

///////////////////////////////////////////////////////////////////////////////

bool DgSorter::isPending(dgmoduledata_ptr_t mod) const {
  return (_pending.find(mod) != _pending.end());
}

//////////////////////////////////////////////////////////

size_t DgSorter::numPending() const {
  return _pending.size();
}

//////////////////////////////////////////////////////////

int DgSorter::numDownstream(dgmoduledata_ptr_t mod) const {
  int inumoutcon = 0;
  int inumouts   = mod->numOutputs();
  for (int io = 0; io < inumouts; io++) {
    outplugdata_ptr_t poutplug = mod->output(io);
    inumoutcon += (int)poutplug->numConnections();
  }
  return inumoutcon;
}

//////////////////////////////////////////////////////////

int DgSorter::numPendingDownstream(dgmoduledata_ptr_t mod) const {
  int inumoutcon = 0;
  int inumouts   = mod->numOutputs();
  for (int io = 0; io < inumouts; io++) {
    outplugdata_ptr_t poutplug = mod->output(io);
    size_t inumcon             = poutplug->numConnections();
    for (size_t ic = 0; ic < inumcon; ic++) {
      auto pinplug               = poutplug->connected(ic);
      dgmoduledata_ptr_t pconmod = std::dynamic_pointer_cast<DgModuleData>(pinplug->_parent_module);
      inumoutcon += int(isPending(pconmod));
    }
  }
  return inumoutcon;
}

//////////////////////////////////////////////////////////

void DgSorter::addModule(dgmoduledata_ptr_t mod) {

  auto& node_info     = _nodeinfomap[mod];
  node_info._depth    = 0;
  node_info._serial   = NOSERIAL;
  int inumo           = mod->numOutputs();
  node_info._modifier = s8(-inumo); // TODO: whats the s8 for again? - its important
  for (int io = 0; io < inumo; io++) {
    auto plug_out       = mod->output(io);
    auto& plug_info     = _pluginfomap[plug_out];
    plug_info._register = nullptr;
  }
  _pending.insert(mod);
}

//////////////////////////////////////////////////////////

orkvector<dgregister*> DgSorter::pruneRegisters(dgmoduledata_ptr_t pmod) {
  auto pruned = _dgcontext->prune(pmod);
  for (auto reg_item : pruned) {
    auto plug = reg_item->_plug;
    auto modul = plug->_parent_module;
    _logchannel_reg->log(
        "PRUNE: module<%s> plug<%s> pruned register<%s>", //
        modul->_name.c_str(), //
        plug->_name.c_str(), //
        reg_item->name().c_str());

    auto it_plug_info          = _pluginfomap.find(plug);
    OrkAssert(it_plug_info != _pluginfomap.end());
    auto& plug_info       = it_plug_info->second;
    if(plug_info._register==reg_item){
      plug_info = PlugInfo();
    }

  }
  return pruned;
}

//////////////////////////////////////////////////////////

bool DgSorter::hasPendingInputs(dgmoduledata_ptr_t mod) const {
  bool bhaspending = false;
  int inumins      = mod->numInputs();
  for (int ip = 0; ip < inumins; ip++) {
    auto pinplug = mod->input(ip);
    if (pinplug->_connectedOutput) {
      auto pout    = pinplug->_connectedOutput;
      auto pconcon = typedModuleData<DgModuleData>(pout->_parent_module);
      auto it      = _pending.find(pconcon);
      if (pconcon == mod &&
          typeid(float) == pinplug->GetDataTypeId()) // connected to self and a float plug, must be an internal loop rate plug
      {                                              // pending.erase(it);
        // it = pending.end();
      } else if (it != _pending.end()) {
        bhaspending = true;
      }
    }
  }
  return bhaspending;
}

//////////////////////////////////////////////////////////
void DgSorter::enqueueModule(dgmoduledata_ptr_t pmod, int irecd) {

  _logchannel->log("TOPO: module<%s> enqueued..", pmod->_name.c_str());

  if (_pending.find(pmod) != _pending.end()) { // is pmod pending ?

    if (_modulestack.size()) { // check the top of stack for registers to prune
      dgmoduledata_ptr_t prev = _modulestack.top();
      pruneRegisters(prev);
    }
    _modulestack.push(pmod);

    ///////////////////////////////////
    auto& node_info   = _nodeinfomap[pmod];
    node_info._serial = ++_serial;
    _pending.erase(pmod);

    ///////////////////////////////////

    int inuminps = pmod->numInputs();
    int inumouts = pmod->numOutputs();

    _logchannel->log("TOPO: module<%s> numin<%d> numout<%d>", pmod->_name.c_str(), inuminps, inumouts);

    ///////////////////////////////////
    // assign new registers
    ///////////////////////////////////

    int inumincon = 0;
    for (int ii = 0; ii < inuminps; ii++) {
      auto pinpplug = pmod->input(ii);
      inumincon += int(pinpplug->isConnected());
    }
    for (int io = 0; io < inumouts; io++) {
      auto poutplug = pmod->output(io);
      if (poutplug->isConnected() || (inumincon != 0)) { // if it has input or output connections
        auto& plug_info     = _pluginfomap[poutplug];
        plug_info._register = _dgcontext->alloc(poutplug);
        _logchannel_reg->log(
            "ASSIGN module<%s> outplug<%s> assigned register<%s>", //
            pmod->_name.c_str(),                                 //
            poutplug->_name.c_str(),                             //
            plug_info._register->name().c_str());
      }
    }

    ///////////////////////////////////
    // add dependents to register
    ///////////////////////////////////

    for (int io = 0; io < inumouts; io++) {
      outplugdata_ptr_t poutplug = pmod->output(io);
      auto& plug_info            = _pluginfomap[poutplug];
      auto preg                  = plug_info._register;
      if (preg) {
        size_t inumcon = poutplug->numConnections();
        for (size_t ic = 0; ic < inumcon; ic++) {
          auto pinp = poutplug->connected(ic);
          if (pinp && pinp->_parent_module != pmod) {
            dgmoduledata_ptr_t dmod = typedModuleData<DgModuleData>(pinp->_parent_module);
            preg->_downstream_dependents.insert(dmod);
          }
        }
      }
    }

    _logchannel->log("TOPO: module<%s> completed..", pmod->_name.c_str());

    ///////////////////////////////////
    // completed "pmod"
    //  add downstream modules connected to pmod's outputs
    //  that have no other pending dependencies
    ///////////////////////////////////

    for (int io = 0; io < inumouts; io++) {
      outplugdata_ptr_t poutplug = pmod->output(io);
      auto& plug_info            = _pluginfomap[poutplug];
      if (plug_info._register) {
        size_t inumcon = poutplug->numConnections();
        for (size_t ic = 0; ic < inumcon; ic++) {
          auto pinp = poutplug->connected(ic);
          if (pinp && pinp->_parent_module != pmod) {
            dgmoduledata_ptr_t dmod = typedModuleData<DgModuleData>(pinp->_parent_module);
            if (false == hasPendingInputs(dmod)) {
              enqueueModule(dmod, irecd + 1);
            }
          }
        }
      }
    }

    ///////////////////////////////////
    // prune registers
    ///////////////////////////////////

    if (_pending.size() != 0) {
      pruneRegisters(pmod);
    }

    ///////////////////////////////////
    _modulestack.pop();
  }
}

//////////////////////////////////////////////////////////

void DgSorter::dumpOutputs(dgmoduledata_ptr_t mod) const {
  _logchannel->log("///////////////////////////");
  _logchannel->log("output dump");
  int inump = mod->numOutputs();
  for (int ip = 0; ip < inump; ip++) {
    outplugdata_ptr_t poutplug = mod->output(ip);
    auto it_plug_info          = _pluginfomap.find(poutplug);
    OrkAssert(it_plug_info != _pluginfomap.end());
    auto& plug_info       = it_plug_info->second;
    dgregister* preg      = plug_info._register;
    dgregisterblock* pblk = (preg != nullptr) ? preg->mpBlock : nullptr;
    std::string regb      = (pblk != nullptr) ? pblk->name() : "";
    int reg_index         = (preg != nullptr) ? preg->mIndex : -1;
    _logchannel->log("  mod<%s> out<%d> reg<%s:%d>", mod->_name.c_str(), ip, regb.c_str(), reg_index);
  }
}

//////////////////////////////////////////////////////////

void DgSorter::dumpInputs(dgmoduledata_ptr_t mod) const {
  _logchannel->log("///////////////////////////");
  _logchannel->log("input dump");
  int inumins = mod->numInputs();
  for (int ip = 0; ip < inumins; ip++) {
    const auto pinplug = mod->input(ip);
    if (pinplug->_connectedOutput) {
      auto poutplug     = pinplug->_connectedOutput;
      auto pconcon      = typedModuleData<DgModuleData>(poutplug->_parent_module);
      auto it_plug_info = _pluginfomap.find(poutplug);
      auto& plug_info   = it_plug_info->second;
      OrkAssert(it_plug_info != _pluginfomap.end());
      dgregister* preg = plug_info._register;
      if (preg) {
        dgregisterblock* pblk = preg->mpBlock;
        std::string regb      = (pblk != nullptr) ? pblk->name() : "";
        int reg_index         = preg->mIndex;
        _logchannel->log(
            "  mod<%s> inp<%d:%s> <<< module<%s> out<%s> reg<%s:%d>", //
            mod->_name.c_str(), //
            ip, //
            pinplug->_name.c_str(), //
            pconcon->_name.c_str(), //
            poutplug->_name.c_str(), //
            regb.c_str(), reg_index); 
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
topology_ptr_t DgSorter::generateTopology() {

  if (not _graphdata->isComplete()) {
    return nullptr;
  }

  auto new_topo = std::make_shared<Topology>();

  using dgmodlut_t = std::multimap<int, dgmoduledata_ptr_t>;

  while (this->numPending()) {
    dgmodlut_t pending_and_ready;

    for (dgmoduledata_ptr_t pmod : this->_pending) {
      if (not hasPendingInputs(pmod)) {

        auto it_node_info = _nodeinfomap.find(pmod);

        const auto& node_info = it_node_info->second;

        int ikey = (node_info._depth * 16) + node_info._modifier;

        pending_and_ready.insert(std::make_pair(ikey, pmod));
      }
    }

    for (const auto& next : pending_and_ready) {
      this->enqueueModule(next.second, 0);
    }
    ///////////////////////////////////////
  }
  ///////////////////////////////////////
  // SORT into flattened
  ///////////////////////////////////////
  size_t num_modules = _graphdata->numModules();
  std::multimap<size_t, dgmoduledata_ptr_t> sorted;
  for (size_t ic = 0; ic < num_modules; ic++) {
    auto module           = _graphdata->module(ic);
    auto it_node_info     = _nodeinfomap.find(module);
    const auto& node_info = it_node_info->second;
    size_t iserial        = node_info._serial;
    sorted.insert(std::make_pair(iserial, module));
  }
  _logchannel->log("///////////////////////////");
  _logchannel->log("TOPO: flattened");
  _logchannel->log("///////////////////////////");

  int index = 0;
  for (auto item : sorted) {
    _logchannel->log("TOPO: index<%d> module<%s>", index, item.second->_name.c_str() );
    new_topo->_flattened.push_back(item.second);
    index++;
  }

  _logchannel->log("///////////////////////////");

  ///////////////////////////////////////
  ///////////////////////////////////////

  return new_topo;
}
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::dataflow
///////////////////////////////////////////////////////////////////////////////