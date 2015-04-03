////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <QtGui/QInputDialog>
//#include <QtGui/ButtonState>
#include <orktool/ged/ged_io.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/kernel/core_interface.h>

namespace ork { namespace tool { namespace ged {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename T> 
Slider<T>::Slider( T& ParentW, datatype min, datatype max, datatype def )
	: SliderBase()
	, mParent(ParentW)
	, mval(def)
	, mmin(min)
	, mmax(max)
	, mbUpdateOnDrag(false)
{
	CPropType<datatype>::ToString( mval,mValStr );

	datatype val = def;

	if( val<mmin ) val = mmin;
	if( val>mmax ) val = mmax;

	float fx = mlogmode ? ValToLog(mval) : ValToLin(mval);
	
	PropTypeString outstr;
	CPropType<datatype>::ToString( val, outstr );

	mval = val;

	Refresh();
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
float Slider<T>::LogToVal( float flog ) const		
{
	const float klogoffset = std::abs(float(mmax)-float(mmin))/100.0f;
	float flogmin = log10f( klogoffset+float(mmin) );	
	float flogmax = log10f( klogoffset+float(mmax) );	
	float flogrange = flogmax-flogmin;				

	float flogdist = (flog*flogrange);				
	float flogdist2 = flogdist+flogmin;				

	// 10^0.0413927 = 1.1
	// 10^1.0413927 = 11

	float flin = powf( 10.0f, flogdist2 );

	return flin-klogoffset;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
float Slider<T>::ValToLog( float val ) const
{
	const float klogoffset = std::abs(float(mmax)-float(mmin))/100.0f;
	float flogmin = log10f( klogoffset+float(mmin) );	
	float flogmax = log10f( klogoffset+float(mmax) );	
	float flogrange = flogmax-flogmin;				

	float flogval = log10f( klogoffset+float(val) );		
	float flogdist = (flogval-flogmin);				

	float fi = flogdist/flogrange;					

	return fi;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
float Slider<T>::LinToVal( float lin ) const
{
	float frange = float(mmax-mmin);
	float fval = (lin*frange)+mmin;
	return fval;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
float Slider<T>::ValToLin( float val ) const
{
	float frange = float(mmax-mmin);
	float flin = (float(val)-float(mmin))/frange;
	return flin;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
void Slider<T>::Refresh()
{
	float fx = mlogmode ? ValToLog(mval) : ValToLin(mval);

	mfIndicPos = (fx*mfw)+mfx;

	float ftextX = 0.0f;
	if( fx<0.6f )
	{
		ftextX = 0.66f;
	}
	else
	{
		ftextX = 0.16f;
	}

	mfTextPos = (ftextX*mfw)+mfx;
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
void Slider<T>::SetVal( datatype val )
{
	if( val<mmin ) val = mmin;
	if( val>mmax ) val = mmax;
	mval = val;
	mParent.RefIODriver().SetValue( val );
	CPropType<datatype>::ToString( mval,mValStr );
	Refresh();
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
void Slider<T>::OnMouseReleased(const ork::ui::Event& ev) 
{
	SetVal(mval);
	IoDriverBase& iod = mParent.RefIODriver();
	mParent.SigInvalidateProperty();
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
void Slider<T>::OnMouseDoubleClicked(const ork::ui::Event& ev) 
{
	std::string RangeStr = CreateFormattedString( "v:[%f..%f]", float(mmin), float(mmax) );
	//QString qstr = QInputDialog::getText ( 0, "Set Value", RangeStr.c_str() );

	int ilabw = mParent.GetNameWidth()+16;
	
	int iwidth = mParent.width()-ilabw;
	if( iwidth<64 ) iwidth=64;
	
	int iheight = mParent.height()-3;
	if( iheight<12 ) iheight=12;
	
	//T val = GetVal();

	//mIoDriver.GetValue(val);
	PropTypeString ptsg;
	CPropType<datatype>::ToString( mval, ptsg );

	//QString qstr = GedInputDialog::getText ( event, & mParent, ptsg.c_str(), 2, 2, mParent.width()-3, miLabelH );
	QString qstr = GedInputDialog::getText ( ev.miX, ev.miY, & mParent, ptsg.c_str(), 2, 2, mParent.width()-3, iheight );

	std::string sstr = qstr.toAscii().data();
	if( sstr.length() )
	{
		PropTypeString pts( sstr.c_str() );
		datatype val = CPropType<datatype>::FromString( pts );
		if( val<mmin ) val = mmin;
		if( val>mmax ) val = mmax;
		mval = val;
		SetVal(mval);
		IoDriverBase& iod = mParent.RefIODriver();
		mParent.SigInvalidateProperty();
	}
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
void Slider<T>::OnMouseMoved(const ork::ui::Event& ev)
{
	mbUpdateOnDrag = ev.mbCTRL;

	bool bleft = ev.IsButton0DownF();
	bool bright = ev.IsButton2DownF();

	if( bleft||bright )
	{
		int mousepos = ev.miX;
		float fx = float((mousepos)-mfx)/mfw;
		float fval = mlogmode ? LogToVal( fx ) : LinToVal( fx );
		datatype dx = datatype(fval); 
		
		if( bright )
		{
			dx = datatype( float(mval)*0.9f+float(dx)*0.1f );
		}
		
		mval = dx;
		if( mval<mmin ) mval = mmin;
		if( mval>mmax ) mval = mmax;
		CPropType<datatype>::ToString( mval,mValStr );
		Refresh();

		if( mbUpdateOnDrag )
		{
			SetVal(mval);
			IoDriverBase& iod = mParent.RefIODriver();
			mParent.SigInvalidateProperty();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

template <typename T> 
void Slider<T>::resize(int ix, int iy, int iw, int ih)
{
	mfx = ix;
	mfw = iw;
	mfh = ih;
	Refresh();
}

///////////////////////////////////////////////////////////////////////////////

template<typename Setter>
GedBoolNode<Setter>::GedBoolNode(ObjModel &mdl, const char *name, const reflect::IObjectProperty *prop, ork::Object *obj)
	: GedItemNode(mdl, name, prop, obj)
	, mSetter(prop, obj)
{
}

///////////////////////////////////////////////////////////////////////////////
static const int CHECKBOX_MARGIN = 2;
#define CHECKBOX_SIZE(height)		(height - CHECKBOX_MARGIN * 2)
#define CHECKBOX_X(x, width, SIZE)	(x + width - SIZE - CHECKBOX_MARGIN)
#define CHECKBOX_Y(y)				(y + CHECKBOX_MARGIN)

template<typename Setter>
void GedBoolNode<Setter>::DoDraw( lev2::GfxTarget* pTARG )
{
	//GedItemNode::DoDraw( pTARG );

	const int SIZE = CHECKBOX_SIZE(miH);
	const int X = CHECKBOX_X(miX, miW, SIZE);
	const int Y = CHECKBOX_Y(miY);

	GetSkin()->DrawBgBox(this, X, Y, SIZE, SIZE, GedSkin::ESTYLE_BACKGROUND_2);

	bool value = false;
	mSetter.GetValue(value);
	if(value)
		GetSkin()->DrawCheckBox( this, miX + miW - SIZE - CHECKBOX_MARGIN, miY + CHECKBOX_MARGIN, SIZE, SIZE);

	GetSkin()->DrawText( this, miX+4, miY+2, mName.c_str() );
}

///////////////////////////////////////////////////////////////////////////////

template<typename Setter>
void GedBoolNode<Setter>::OnMouseReleased(const ork::ui::Event& ev)
{
	int evx = ev.miX;
	int evy = ev.miY;

	const int SIZE = CHECKBOX_SIZE(miH);
	const int X = CHECKBOX_X(miX, miW, SIZE);
	const int Y = CHECKBOX_Y(miY);

	if(evx > X && evx < X + SIZE
		&& evy > Y && evy < Y + SIZE)
	{
		bool value = false;
		mSetter.GetValue(value);
		mSetter.SetValue(!value);
	}
	SigInvalidateProperty();
}

///////////////////////////////////////////////////////////////////////////////

template<typename Setter>
void GedBoolNode<Setter>::OnMouseDoubleClicked(const ork::ui::Event& ev)
{
	bool value = false;
	mSetter.GetValue(value);
	mSetter.SetValue(!value);
	SigInvalidateProperty();
}

///////////////////////////////////////////////////////////////////////////////

template<typename IODriver>
GedFloatNode<IODriver>::GedFloatNode(	ObjModel& mdl, const char* name, const reflect::IObjectProperty* prop, ork::Object* obj )
	: GedItemNode( mdl, name,prop,obj)
	, mIoDriver( mdl, prop, obj )
	, mLogMode( false )
{
	ConstString annomin = prop->GetAnnotation( "editor.range.min" );
	ConstString annomax = prop->GetAnnotation( "editor.range.max" );
	ConstString annolog = prop->GetAnnotation( "editor.range.log" );

	float fval = -1.0f;

	mIoDriver.GetValue( fval );

	float fmin = 0.0f;
	float fmax = 1.0f;

	if( annomin.length() )	{ sscanf( annomin.c_str(), "%f", & fmin ); }
	if( annomax.length() )	{ sscanf( annomax.c_str(), "%f", & fmax ); }
	if( annolog.length() )
	{
		if( annolog == "true" )
		{
			mLogMode = true; 
		}
	}

	slider = new Slider<GedFloatNode>( *this, fmin, fmax, fval );
	slider->SetLogMode(mLogMode);
}

template<typename IODriver>
void GedFloatNode<IODriver>::ReSync() // virtual
{
	float fval = -1.0f;
	mIoDriver.GetValue( fval );
	slider->SetVal( fval );
}

template<typename IoDriver>
void GedFloatNode<IoDriver>::DoDraw( lev2::GfxTarget* pTARG )
{
	slider->resize(miX,miY,miW,miH);

	int ixi = int(slider->GetIndicPos()) - miX;
	GetSkin()->DrawBgBox( this, miX, miY, miW, miH, GedSkin::ESTYLE_BACKGROUND_1 );
	GetSkin()->DrawBgBox( this, miX+2, miY+2, miW-3, miH-4, GedSkin::ESTYLE_BACKGROUND_2 );
	GetSkin()->DrawBgBox( this, miX+2, miY+4, ixi, miH-8, GedSkin::ESTYLE_DEFAULT_HIGHLIGHT );

	ork::PropTypeString pstr;
	float fval = 0.0f;
	mIoDriver.GetValue(fval);
	float finp = slider->GetTextPos();
	int itxi = miX+(finp);
	PropTypeString& str = slider->ValString();

	mLabel = str.c_str();
	int itextlen = GetLabelWidth();

	int ity = get_text_center_y();

	GetSkin()->DrawText( this, miX+miW-(itextlen+8), ity, str.c_str() );
	GetSkin()->DrawText( this, miX+4, ity, mName.c_str() );
}

///////////////////////////////////////////////////////////////////////////////

template<typename IODriver>
GedIntNode<IODriver>::GedIntNode(	ObjModel& mdl, const char* name, const reflect::IObjectProperty* prop, ork::Object* obj )
	: GedItemNode( mdl, name,prop,obj)
	, mIoDriver( mdl, prop, obj )
	, mLogMode( false )
{
	ConstString annomin = prop->GetAnnotation( "editor.range.min" );
	ConstString annomax = prop->GetAnnotation( "editor.range.max" );
	ConstString annolog = prop->GetAnnotation( "editor.range.log" );

	int ival = -1;

	mIoDriver.GetValue( ival );

	int fmin = 0.0f;
	int fmax = 1.0f;
	if( annomin.length() )	{ sscanf( annomin.c_str(), "%d", & fmin ); }
	if( annomax.length() )	{ sscanf( annomax.c_str(), "%d", & fmax ); }

	if( annolog.length() )
	{
		if( annolog == "true" )
		{
			mLogMode = true; 
		}
	}

	slider = new Slider<GedIntNode>( *this, fmin, fmax, ival );
	SetName( name );
}

template<typename IODriver>
void GedIntNode<IODriver>::ReSync() // virtual
{
	int ival = 9;
	mIoDriver.GetValue( ival );
	((Slider<GedIntNode>*)slider)->SetVal( ival );
}

template<typename IODriver>
void GedIntNode<IODriver>::DoDraw( lev2::GfxTarget* pTARG )
{
	slider->resize(miX,miY,miW,miH);

	int ixi = int(slider->GetIndicPos()) - miX;
	GetSkin()->DrawBgBox( this, miX, miY, miW, miH, GedSkin::ESTYLE_BACKGROUND_1 );
	GetSkin()->DrawBgBox( this, miX+2, miY+2, miW-3, miH-4, GedSkin::ESTYLE_BACKGROUND_2 );
	GetSkin()->DrawBgBox( this, miX+2, miY+3, ixi, miH-6, GedSkin::ESTYLE_DEFAULT_HIGHLIGHT );

	int ity = get_text_center_y();

	ork::PropTypeString pstr;
	int ival = 0;
	mIoDriver.GetValue(ival);
	float finp = slider->GetTextPos();
	int itxi = miX+int(finp);
	PropTypeString& str = slider->ValString();
	GetSkin()->DrawText(  this, itxi, ity, str.c_str() );
	mLabel = str.c_str();
	int itextlen = GetLabelWidth();
	GetSkin()->DrawText( this, miX+4, ity, mName.c_str() );
//	GetSkin()->DrawText( this, miX+miW-(itextlen+8), miY+4, str.c_str() );
}

///////////////////////////////////////////////////////////////////////////////

template<typename IODriver>
void GedIntNode<IODriver>::OnMouseMoved(const ork::ui::Event& ev)
{
	slider->OnMouseMoved(ev);
	mModel.SigRepaint();
}

///////////////////////////////////////////////////////////////////////////////

template<typename IODriver>
void GedIntNode<IODriver>::OnMouseDoubleClicked(const ork::ui::Event& ev) 
{
	slider->OnMouseDoubleClicked(ev);
	mModel.SigRepaint();
}

///////////////////////////////////////////////////////////////////////////////

template<typename IODriver>
void GedIntNode<IODriver>::OnMouseReleased(const ork::ui::Event& ev)
{
	slider->OnMouseReleased(ev);
	mModel.SigRepaint();
}

///////////////////////////////////////////////////////////////////////////////

template<typename IoDriver>
void GedFloatNode<IoDriver>::OnMouseMoved(const ork::ui::Event& ev) 
{
	slider->OnMouseMoved(ev);
	mModel.SigRepaint();
}

///////////////////////////////////////////////////////////////////////////////

template<typename IoDriver>
void GedFloatNode<IoDriver>::OnMouseDoubleClicked(const ork::ui::Event& ev) 
{
	slider->OnMouseDoubleClicked(ev);
	mModel.SigRepaint();
}

///////////////////////////////////////////////////////////////////////////////

template<typename IoDriver>
void GedFloatNode<IoDriver>::OnMouseReleased(const ork::ui::Event& ev)
{
	slider->OnMouseReleased(ev);
	mModel.SigRepaint();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template<typename IODriver,typename T>
GedSimpleNode<IODriver,T>::GedSimpleNode(ObjModel& mdl, const char* name, const reflect::IObjectProperty* prop, ork::Object* obj )
	: GedItemNode( mdl, name, prop, obj )
	, mIoDriver( mdl, prop, obj )
{
	
}
///////////////////////////////////////////////////////////////////////////////
template<typename IODriver,typename T>
void GedSimpleNode<IODriver,T>::OnMouseDoubleClicked(const ork::ui::Event& ev)
{
	T val;

	mIoDriver.GetValue(val);
	PropTypeString ptsg;
	CPropType<T>::ToString( val, ptsg );

	ConstString anno_ucdclass = GetOrkProp()->GetAnnotation( "ged.userchoice.delegate" );

	if( anno_ucdclass.length() )
	{
		ork::Object* pobj = GetOrkObj();
	
		rtti::Class *the_class = rtti::Class::FindClass(anno_ucdclass);
		if( the_class )
		{	ork::object::ObjectClass* pucdclass = rtti::autocast(the_class);
			ork::rtti::ICastable* ucdo = the_class->CreateObject();
			IUserChoiceDelegate* ucd = rtti::autocast(ucdo);
			if( ucd )
			{	UserChoices uchc( *ucd, pobj, this );
				QMenu* qm = uchc.CreateMenu();
				QAction* pact = qm->exec(QCursor::pos());
				if( pact )
				{
					QVariant UserData = pact->data();
					QString UserName = UserData.toString();
					std::string pname = UserName.toAscii().data();
					
					const CAttrChoiceValue *Chc = uchc.FindFromLongName(pname);

					if( Chc )
					{
						if( Chc->GetCustomData().IsA<T>() )
						{
							const T& value = Chc->GetCustomData().Get<T>();
						}
						std::string valuestr = Chc->EvaluateValue();
						PropTypeString pts( valuestr.c_str() );
						val = CPropType<T>::FromString( pts );
						mIoDriver.SetValue(val);
					}
				}
			}
		}
	}
	else
	{	int ilabw = GetNameWidth()+16;
		QString qstr = GedInputDialog::getText ( ev.miX, ev.miY, this, ptsg.c_str(), ilabw, 2, miW-ilabw, miH-3 );
		std::string sstr = qstr.toAscii().data();
		if( sstr.length() )
		{	PropTypeString pts( sstr.c_str() );
			val = CPropType<T>::FromString( pts );
			mIoDriver.SetValue(val);
		}
	}
	SigInvalidateProperty();
}
///////////////////////////////////////////////////////////////////////////////
template<typename IODriver,typename T>
void GedSimpleNode<IODriver,T>::DoDraw( lev2::GfxTarget* pTARG )
{
	bool IsPickState = pTARG->FBI()->IsPickState();

	int ity = get_text_center_y();

	int ilabw = GetNameWidth()+16;
	//GedItemNode::DoDraw( pTARG );
	//////////////////////////////////////
	T val;
	mIoDriver.GetValue(val);
	GetSkin()->DrawBgBox( this, miX+ilabw, miY, miW-ilabw, miH, GedSkin::ESTYLE_BACKGROUND_2 );

	if( false==IsPickState )
	{
		PropTypeString pts;
		CPropType<T>::ToString( val, pts );
		//////////////////////////////////////

		GetSkin()->DrawText( this, miX+ilabw, ity, pts.c_str() );
		GetSkin()->DrawText( this, miX+6, ity, mName.c_str() );
	}

}
///////////////////////////////////////////////////////////////////////////////
} } }
///////////////////////////////////////////////////////////////////////////////
