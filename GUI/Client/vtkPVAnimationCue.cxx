/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationCue.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationCue.h"

#include "vtkObjectFactory.h"
#include "vtkAnimationCue.h"
#include "vtkKWApplication.h"
#include "vtkPVTimeLine.h"
#include "vtkKWLabel.h"
#include "vtkKWFrame.h"
#include "vtkCommand.h"
#include "vtkKWParameterValueFunctionEditor.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWEvent.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMKeyFrameAnimationCueManipulatorProxy.h"
#include "vtkPVKeyFrame.h"
#include "vtkPVRampKeyFrame.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMKeyFrameProxy.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVVerticalAnimationInterface.h"
#include "vtkPVAnimationScene.h"
#include "vtkPVSource.h"
#include "vtkSMPropertyStatusManager.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkKWRange.h"

#define VTK_PV_ANIMATON_ENTRY_HEIGHT 20

/* 
 * This part was generated by ImageConvert from image:
 *    close.png (zlib, base64)
 */
#define image_close_width         9
#define image_close_height        9
#define image_close_pixel_size    4
#define image_close_buffer_length 48

static unsigned char image_close[] = 
  "eNpjYGD4z0AEBgIGXBibGmx8UtTgcgMt7CLkL0IYANH+oGA=";


/* 
 * This part was generated by ImageConvert from image:
 *    open.png (zlib, base64)
 */
#define image_open_width         9
#define image_open_height        9
#define image_open_pixel_size    4
#define image_open_buffer_length 40

static unsigned char image_open[] = 
  "eNpjYGD4z0AEBgIGXJgWanC5YSDcQwgDAO0pqFg=";

vtkStandardNewMacro(vtkPVAnimationCue);
vtkCxxRevisionMacro(vtkPVAnimationCue, "1.6");
vtkCxxSetObjectMacro(vtkPVAnimationCue, TimeLineParent, vtkKWWidget);

//***************************************************************************
class vtkPVAnimationCueObserver : public vtkCommand
{
public:
  static vtkPVAnimationCueObserver* New()
    {return new vtkPVAnimationCueObserver;}

  void SetAnimationCue(vtkPVAnimationCue* proxy)
    {
    this->AnimationCue = proxy;
    }
  virtual void Execute(vtkObject* wdg, unsigned long event,
    void* calldata)
    {
    if (this->AnimationCue)
      {
      this->AnimationCue->ExecuteEvent(wdg, event, calldata);
      }
    }
protected:
  vtkPVAnimationCueObserver()
    {
    this->AnimationCue = 0;
    }
  vtkPVAnimationCue* AnimationCue;
};
//***************************************************************************

//-----------------------------------------------------------------------------
vtkPVAnimationCue::vtkPVAnimationCue()
{
  this->Observer = vtkPVAnimationCueObserver::New();
  this->Observer->SetAnimationCue(this);
  this->TimeLineParent = NULL;
  this->LabelText = NULL;
  this->TimeLineContainer = vtkKWFrame::New();
  this->Label = vtkKWLabel::New();
  this->TimeLine = vtkPVTimeLine::New();
  this->TimeLine->SetTraceReferenceObject(this);
  this->TimeLine->SetTraceReferenceCommand("GetTimeLine");

  this->ImageType = vtkPVAnimationCue::NONE;
  this->Image = vtkKWLabel::New();
  this->Frame = vtkKWFrame::New();
  this->TimeLineFrame = vtkKWFrame::New();
  this->ShowTimeLine = 1;
  this->Focus = 0;
  this->Virtual = 0;
  this->NumberOfPoints = 0;
  this->PointParameters[0] = this->PointParameters[1] = 0.0;

  this->CueProxy = 0;
  this->CueProxyName = 0;
  this->KeyFrameManipulatorProxy = 0;
  this->KeyFrameManipulatorProxyName = 0;

  this->PVKeyFrames = vtkCollection::New();
  this->PVKeyFramesIterator = this->PVKeyFrames->NewIterator();

  this->PVAnimationScene = NULL;
  this->PVSource = NULL;
  this->ProxiesRegistered = 0;

  this->PropertyStatusManager = NULL;
  this->Name = NULL;
}


//-----------------------------------------------------------------------------
vtkPVAnimationCue::~vtkPVAnimationCue()
{
  this->SetPVSource(NULL);
  this->UnregisterProxies();

  this->Observer->SetAnimationCue(NULL);
  this->Observer->Delete();
  this->SetLabelText(0);
  this->SetTimeLineParent(0);
  this->TimeLineContainer->Delete();
  this->Label->Delete();
  this->TimeLine->Delete();
  this->Image->Delete();
  this->Frame->Delete();
  this->TimeLineFrame->Delete();
  this->PVKeyFrames->Delete();
  this->PVKeyFramesIterator->Delete();

  this->SetCueProxyName(0);
  if (this->CueProxy)
    {
    this->CueProxy->Delete();
    this->CueProxy = 0;
    }

  this->SetKeyFrameManipulatorProxyName(0);
  if (this->KeyFrameManipulatorProxy)
    {
    this->KeyFrameManipulatorProxy->Delete();
    this->KeyFrameManipulatorProxy = 0;
    }
  this->SetAnimationScene(NULL);

  if (this->PropertyStatusManager)
    {
    this->PropertyStatusManager->Delete();
    this->PropertyStatusManager = NULL;
    }
  this->SetName(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetVirtual(int v)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("Virtual state can only be changed before creation.");
    return;
    }
  this->Virtual = v;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetPVSource(vtkPVSource* src)
{
  //Should not be reference counted to avoid cycles.
  this->PVSource = src;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetAnimationScene(vtkPVAnimationScene* scene)
{
  this->PVAnimationScene = scene;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::RegisterProxies()
{
  if (this->Virtual || !this->CueProxyName || !this->KeyFrameManipulatorProxyName)
    {
    return;
    }

  if (this->ProxiesRegistered)
    {
    return;
    }
  vtkSMObject::GetProxyManager()->RegisterProxy("animation",
    this->CueProxyName, this->CueProxy);
  vtkSMObject::GetProxyManager()->RegisterProxy("animation_manipulators",
    this->KeyFrameManipulatorProxyName, this->KeyFrameManipulatorProxy);

  if (this->PVAnimationScene)
    {
    this->PVAnimationScene->AddAnimationCue(this);
    }
  this->ProxiesRegistered = 1;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::UnregisterProxies()
{
  if (this->Virtual || !this->CueProxyName || !this->KeyFrameManipulatorProxyName)
    {
    return;
    }
  if (!this->ProxiesRegistered)
    {
    return;
    }
  vtkSMObject::GetProxyManager()->UnRegisterProxy("animation",
    this->CueProxyName);
  vtkSMObject::GetProxyManager()->UnRegisterProxy("animation_manipulators",
    this->KeyFrameManipulatorProxyName);
  if (this->PVAnimationScene)
    {
    this->PVAnimationScene->RemoveAnimationCue(this);
    }
  this->ProxiesRegistered = 0;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetEnableZoom(int zoom)
{
  this->TimeLine->SetShowParameterRange(zoom);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::CreateProxy()
{
  if (!this->Virtual)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    static int proxyNum = 0;
    this->CueProxy = vtkSMAnimationCueProxy::SafeDownCast(
      pxm->NewProxy("animation","AnimationCue"));
    if (!this->CueProxy)
      {
      vtkErrorMacro("Failed to create proxy " << "AnimationCue");
      return;
      }
    ostrstream str;
    str << "vtkPVTimeLine_AnimationCue" << proxyNum << ends;
    this->SetCueProxyName(str.str());

    this->KeyFrameManipulatorProxy = vtkSMKeyFrameAnimationCueManipulatorProxy::
      SafeDownCast(pxm->NewProxy("animation_manipulators",
          "KeyFrameAnimationCueManipulator"));
    if (!this->KeyFrameManipulatorProxy)
      {
      vtkErrorMacro("Failed to create proxy KeyFrameAnimationCueManipulator");
      return;
      }
    ostrstream str1;
    str1 << "vtkPVTimeLine_KeyFrameAnimationCueManipulator" << proxyNum << ends;
    this->SetKeyFrameManipulatorProxyName(str1.str());

    proxyNum++;
    str.rdbuf()->freeze(0);
    str1.rdbuf()->freeze(0);


    this->KeyFrameManipulatorProxy->UpdateVTKObjects();

    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->CueProxy->GetProperty("Manipulator"));
    if (pp)
      {
      pp->RemoveAllProxies();
      pp->AddProxy(this->KeyFrameManipulatorProxy);
      }
    this->CueProxy->UpdateVTKObjects(); //calls CreateVTKObjects(1) internally.
    this->CueProxy->SetTimeMode(VTK_ANIMATION_CUE_TIMEMODE_NORMALIZED);
    this->CueProxy->SetStartTime(0);
    this->CueProxy->SetEndTime(1);

    this->KeyFrameManipulatorProxy->AddObserver(
      vtkSMKeyFrameAnimationCueManipulatorProxy::StateModifiedEvent, this->Observer);
    this->KeyFrameManipulatorProxy->AddObserver(
      vtkCommand::ModifiedEvent, this->Observer);
    }
}

//-----------------------------------------------------------------------------
unsigned long vtkPVAnimationCue::GetKeyFramesMTime()
{
  return (this->Virtual)? this->GetMTime() :
      this->KeyFrameManipulatorProxy->GetMTime();
}

//-----------------------------------------------------------------------------
int vtkPVAnimationCue::GetNumberOfKeyFrames()
{
  return (this->Virtual)? this->NumberOfPoints :
    this->KeyFrameManipulatorProxy->GetNumberOfKeyFrames();
}

//-----------------------------------------------------------------------------
double vtkPVAnimationCue::GetKeyFrameTime(int id)
{
  if (id < 0 || id >= this->GetNumberOfKeyFrames())
    {
    vtkErrorMacro("Id beyond range");
    return 0.0;
    }
  if (this->Virtual)
    {
    return this->PointParameters[id];
    }
  else
    {
    vtkSMKeyFrameProxy* keyframe = this->KeyFrameManipulatorProxy->
      GetKeyFrameAtIndex(id);
    if (!keyframe)
      {
      vtkErrorMacro("Failed to get keyframe for index " << id );
      return 0.0;
      }
    return keyframe->GetKeyTime();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetKeyFrameTime(int id, double time)
{
  if (id < 0 || id >= this->GetNumberOfKeyFrames())
    {
    vtkErrorMacro("Id beyond range");
    return;
    }
  if (this->Virtual)
    {
    this->PointParameters[id] = time;
    this->Modified(); // Since the function modifed time in Virtual mode is
                      // PVCue modified time.
    this->InvokeEvent(vtkPVAnimationCue::KeysModifiedEvent);
    }
  else
    {
    vtkSMKeyFrameProxy* keyframe = this->KeyFrameManipulatorProxy->
      GetKeyFrameAtIndex(id);
     if (!keyframe)
      {
      vtkErrorMacro("Failed to get keyframe for index " << id );
      return;
      }
     keyframe->SetKeyTime(time);
    }
  this->AddTraceEntry("$kw(%s) SetKeyFrameTime %d %f", this->GetTclName(),
    id, time);
}

//-----------------------------------------------------------------------------
int vtkPVAnimationCue::AddNewKeyFrame(double time)
{
  int id = -1;
  if (this->Virtual)
    {
    if (this->NumberOfPoints >= 2)
      {
      vtkErrorMacro("When PVCue doesn't have a proxy associated with it "
        "it can only have two points.");
      return id;
      }
    this->PointParameters[this->NumberOfPoints] = time;
    id = this->NumberOfPoints;
    this->NumberOfPoints++;
    this->Modified(); // Since the function modifed time in Virtual mode is
                      // PVCue modified time.
    this->InvokeEvent(vtkPVAnimationCue::KeysModifiedEvent);
    }
  else
    {
    id = this->CreateAndAddKeyFrame(time, vtkPVAnimationManager::RAMP);
    }
  return id;
}

//-----------------------------------------------------------------------------
int vtkPVAnimationCue::CreateAndAddKeyFrame(double time, int type)
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(
    this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 

  // First, synchronize the system state to the keyframe time to get proper
  // domain and property values.
  pvAM->SetCurrentTime(time);

  this->AddTraceEntry("$kw(%s) CreateAndAddKeyFrame %f %d",
    this->GetTclName(), time, type);
  
  static int num_objects = 0;
  ostrstream str ;
  str << "KeyFrameName_" << num_objects++ << ends;
  
  vtkPVKeyFrame* keyframe = pvAM->NewKeyFrame(type);
  keyframe->SetName(str.str());
  str.rdbuf()->freeze(0);
  
  keyframe->SetTraceReferenceObject(this);
  ostrstream sCommand;
  sCommand << "GetKeyFrame \"" << keyframe->GetName() << "\"" << ends;
  keyframe->SetTraceReferenceCommand(sCommand.str());
  sCommand.rdbuf()->freeze(0);

  keyframe->SetAnimationCue(this); // provide a pointer to cue, so that the interace
  // can be in accordance with the animated proeprty.
  keyframe->Create(this->GetApplication(),NULL);
  keyframe->SetKeyTime(time);
  keyframe->SetKeyValue(0);
  int id = this->AddKeyFrame(keyframe);
  keyframe->Delete();

  this->InitializeKeyFrameUsingCurrentState(keyframe);



  this->TimeLine->SelectPoint(id);
  return id;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::InitializeKeyFrameUsingCurrentState(vtkPVKeyFrame* keyframe)
{
  keyframe->InitializeKeyValueUsingCurrentState();
  keyframe->InitializeKeyValueDomainUsingCurrentState();
}

//-----------------------------------------------------------------------------
int vtkPVAnimationCue::AddKeyFrame(vtkPVKeyFrame* keyframe)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Attempt to added keyframe to a Virtual Cue");
    return -1;
    }
  if (!keyframe)
    {
    return -1;
    }
  if (this->PVKeyFrames->IsItemPresent(keyframe))
    {
    vtkErrorMacro("Key frame already exists");
    return -1;
    }

  this->PVKeyFrames->AddItem(keyframe);
  return this->KeyFrameManipulatorProxy->AddKeyFrame(keyframe->GetKeyFrameProxy());
}

//-----------------------------------------------------------------------------
int vtkPVAnimationCue::RemoveKeyFrame(int id)
{
  if (id < 0 || id >= this->GetNumberOfKeyFrames())
    {
    return 0;
    }
  if (this->Virtual)
    {
    if (id == 0)
      {
      this->PointParameters[0] = this->PointParameters[1];
      }
    this->NumberOfPoints--;
    this->Modified(); // Since the function modifed time in Virtual mode is
                      // PVCue modified time.
    this->InvokeEvent(vtkPVAnimationCue::KeysModifiedEvent);
    }
  else
    {
    vtkPVKeyFrame* keyframe = this->GetKeyFrame(id);
    this->RemoveKeyFrame(keyframe);
    }
  this->AddTraceEntry("$kw(%s) RemoveKeyFrame %d",
    this->GetTclName(), id);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetTimeMarker(double time)
{
  this->TimeLine->SetTimeMarker(time);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::RemoveKeyFrame(vtkPVKeyFrame* keyframe)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue has no actual keyframes.");
    return;
    }
  if (!keyframe)
    {
    return;
    }
  this->KeyFrameManipulatorProxy->RemoveKeyFrame(keyframe->GetKeyFrameProxy());
  this->PVKeyFrames->RemoveItem(keyframe);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::ReplaceKeyFrame(vtkPVKeyFrame* oldFrame, 
  vtkPVKeyFrame* newFrame)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue has no actual keyframes.");
    return;
    }
  // Removing a point can change its selection. So, we save the current 
  // selection and restore it.
  int selection_id = this->TimeLine->GetSelectedPoint();
  
  newFrame->SetName(oldFrame->GetName());
  newFrame->SetTraceReferenceObject(this);
  ostrstream sCommand;
  sCommand << "GetKeyFrame \"" << newFrame->GetName() << "\"" << ends;
  newFrame->SetTraceReferenceCommand(sCommand.str());
  sCommand.rdbuf()->freeze(0);

  newFrame->SetKeyTime(oldFrame->GetKeyTime());
  newFrame->SetKeyValue(oldFrame->GetKeyValue());
  
  
  this->RemoveKeyFrame(oldFrame);
  this->AddKeyFrame(newFrame);
  this->TimeLine->SelectPoint(selection_id);
}

//-----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVAnimationCue::GetKeyFrame(const char* name)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue has no actual keyframes");
    return NULL;
    }
  if (name == NULL)
    {
    return NULL;
    }
  vtkCollectionIterator* iter = this->PVKeyFramesIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkPVKeyFrame* pvKeyFrame = vtkPVKeyFrame::SafeDownCast(
      iter->GetCurrentObject());
    const char* framename = pvKeyFrame->GetName();
    if (framename && strcmp(framename, name)==0)
      {
      return pvKeyFrame;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVAnimationCue::GetKeyFrame(int id)
{
  if (id < 0 || id >= this->GetNumberOfKeyFrames())
    {
    vtkErrorMacro("Id out of range");
    return NULL;
    }
  if (this->Virtual)
    {
    vtkErrorMacro("Cue has no actual keyframes");
    return NULL;
    }
  vtkSMKeyFrameProxy* kfProxy = this->KeyFrameManipulatorProxy->
    GetKeyFrameAtIndex(id);
  if (!kfProxy)
    {
    vtkErrorMacro("Cannot find keyframe at index " << id );
    return NULL;
    }
  vtkCollectionIterator* iter = this->PVKeyFramesIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
    iter->GoToNextItem())
    {
    vtkPVKeyFrame* pvKeyFrame = vtkPVKeyFrame::SafeDownCast(
      iter->GetCurrentObject());
    if (pvKeyFrame->GetKeyFrameProxy() == kfProxy)
      {
      return pvKeyFrame;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::Create(vtkKWApplication* app, const char* args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("Widget already created.");
    return;
    }

  if (!this->TimeLineParent)
    {
    vtkErrorMacro("TimeLineParent must be set");
    return;
    }
  this->Superclass::Create(app, "frame", args);

  this->CreateProxy();
  
  this->TimeLineContainer->SetParent(this->TimeLineParent);
  this->TimeLineContainer->Create(app, NULL);
  
  this->TimeLineFrame->SetParent(this->TimeLineContainer);
  this->TimeLine->SetParameterCursorInteractionStyle(
    vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleDragWithLeftButton |
    vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleSetWithRighButton |
    vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleSetWithControlLeftButton);

  ostrstream tf_options;
  tf_options << "-height " << VTK_PV_ANIMATON_ENTRY_HEIGHT << ends;
  this->TimeLineFrame->Create(app, tf_options.str());
  tf_options.rdbuf()->freeze(0);
  
  // Create the time line associated with this entry.
  this->TimeLine->SetShowLabel(0);
  this->TimeLine->SetCanvasHeight(VTK_PV_ANIMATON_ENTRY_HEIGHT);
  this->TimeLine->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginHorizontalSides);
  this->TimeLine->SetAnimationCue(this);
  this->TimeLine->SetParent(this->TimeLineFrame);
  this->TimeLine->Create(app, 0);
  this->TimeLine->SetCanvasOutlineStyle(
    vtkKWParameterValueFunctionEditor::CanvasOutlineStyleHorizontalSides |
    vtkKWParameterValueFunctionEditor::CanvasOutlineStyleBottomSide);

  this->Frame->SetParent(this);
  ostrstream frame_options;
  int height = (this->TimeLine->GetShowParameterRange())? 
    this->TimeLine->GetParameterRange()->GetThickness() : 0;
  frame_options << "-relief flat -height " 
    << this->TimeLine->GetCanvasHeight() + height
    << ends;
  this->Frame->Create(app,frame_options.str());
  frame_options.rdbuf()->freeze(0);
  
  this->Label->SetParent(this->Frame);
  this->Label->Create(app, args);
  ostrstream label_text;
  label_text << "-text {" << ((this->LabelText)? this->LabelText:
    "<No Label>" ) 
    << "}"
    << ends;
  this->Label->ConfigureOptions(label_text.str());
  label_text.rdbuf()->freeze(0);

  this->Script("pack propagate %s 0", this->Frame->GetWidgetName());
  this->Script("bind %s <ButtonPress-1> {%s GetFocus}",
    this->Label->GetWidgetName(), this->GetTclName());
  this->Image->SetParent(this->Frame);
  this->Image->Create(app, "-relief flat");
  this->SetImageType(this->ImageType);
  this->InitializeObservers(this->TimeLine);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::InitializeObservers(vtkObject* object)
{
  object->AddObserver(
    vtkKWParameterValueFunctionEditor::PointMovedEvent, this->Observer);
  object->AddObserver(
    vtkKWParameterValueFunctionEditor::PointMovingEvent, this->Observer);

  object->AddObserver(
    vtkKWParameterValueFunctionEditor::SelectionChangedEvent, this->Observer);
  object->AddObserver(vtkKWEvent::FocusInEvent, this->Observer);
  object->AddObserver(vtkKWEvent::FocusOutEvent, this->Observer);
  object->AddObserver(vtkPVAnimationCue::KeysModifiedEvent, this->Observer);

  if (this->TimeLine->GetShowParameterRange())
    {
    object->AddObserver(vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent,
      this->Observer);
    object->AddObserver(vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent,
      this->Observer);
    }
  object->AddObserver(vtkKWParameterValueFunctionEditor::ParameterCursorMovedEvent,
    this->Observer);
  object->AddObserver(vtkKWParameterValueFunctionEditor::ParameterCursorMovingEvent,
    this->Observer);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::Zoom(double range[2])
{
  double old_range[2];
  this->TimeLine->GetVisibleParameterRange(old_range);
  if (old_range[0] != range[0] || old_range[1] != range[1])
    {
    this->TimeLine->SetVisibleParameterRange(range);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata)
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(
    this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 

  if (wdg == this->TimeLine)
    {
    switch(event)
      {
    case vtkKWEvent::FocusInEvent:
      this->GetFocus();
      return;

    case vtkKWEvent::FocusOutEvent:
      // NOTE: we are removing the focus of only self and not that of
      // the children (if any), since the focus presently was on self,
      // otherwise FocusOutEvent would have never been triggerred.
      this->RemoveSelfFocus();
      return;

    case vtkKWParameterValueFunctionEditor::SelectionChangedEvent:
      // raise this event on this cue, so that the VAnimationInterface (if it is listening)
      // will know that selection has changed and will update to show the right
      // key frame.
      this->InvokeEvent(event, calldata);
      return;

    case vtkKWParameterValueFunctionEditor::ParameterCursorMovingEvent:
        {
        double param = this->TimeLine->GetParameterCursorPosition();
        pvAM->SetTimeMarker(param);
        }
      return;
    case vtkKWParameterValueFunctionEditor::ParameterCursorMovedEvent:
        {
        double param = this->TimeLine->GetParameterCursorPosition();
        pvAM->SetCurrentTime(param);
        pvAM->SetTimeMarker(param);
        }
      }
    }
  else if (vtkSMKeyFrameAnimationCueManipulatorProxy::SafeDownCast(wdg))
    {
    switch (event)
      {
    case vtkSMKeyFrameAnimationCueManipulatorProxy::StateModifiedEvent :
      if (this->PVSource && !pvAM->GetUseGeometryCache())
        {
        this->PVSource->MarkSourcesForUpdate();
        }
      return;

    case vtkCommand::ModifiedEvent:
      // Triggerred when the keyframes have been changed in someway.
      this->TimeLine->ForceUpdate();
      if (this->PVAnimationScene)
        {
        this->PVAnimationScene->InvalidateAllGeometries();
        }
      if (this->GetNumberOfKeyFrames() >= 2 )
        {
        this->RegisterProxies();
        }
      if (this->GetNumberOfKeyFrames() < 2)
        {
        this->UnregisterProxies();
        }
      this->InvokeEvent(vtkPVAnimationCue::KeysModifiedEvent);
      return;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::PackWidget()
{
  int label_frame_width = 1;
  if (!this->IsCreated())
    {
    vtkErrorMacro("Widget must be created before packing");
    return;
    }
  
  if (this->ShowTimeLine)
    {
    this->Script("pack %s -anchor n -side top -fill x -expand t",
      this->TimeLine->GetWidgetName());
    }

  this->Script("pack %s -anchor n -side top -fill x -expand t",
    this->TimeLineFrame->GetWidgetName());

  this->Script("pack %s -anchor n -side top -fill x -expand t",
    this->TimeLineContainer->GetWidgetName());


  if (this->ImageType != vtkPVAnimationCue::NONE)
    {
    this->Script("pack %s -anchor w -side left",
      this->Image->GetWidgetName());
    this->Script("winfo reqwidth %s", this->Image->GetWidgetName());
    label_frame_width += vtkKWObject::GetIntegerResult(this->GetApplication());
    }
  
  this->Script("pack %s -anchor w -side left",
    this->Label->GetWidgetName());
  this->Script("winfo reqwidth %s", this->Label->GetWidgetName());
  label_frame_width += vtkKWObject::GetIntegerResult(this->GetApplication());

  this->Script("pack %s -anchor nw -side top -fill x -expand t",
    this->Frame->GetWidgetName());
  
  this->Script("pack %s -anchor n -side top -fill x -expand t",
    this->GetWidgetName());

  // Set the label width properly, since we have disabled pack propagate on the frame
  // containig the label.
  if (label_frame_width != 1)
    {
    ostrstream str;
    str << "-width " << label_frame_width << ends;
    this->Frame->ConfigureOptions(str.str());
    str.rdbuf()->freeze(0);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::UnpackWidget()
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Widget must be created before packing");
    return;
    }
  this->Script("pack forget %s",
    this->TimeLine->GetWidgetName());

  this->Script("pack forget %s",
    this->TimeLineFrame->GetWidgetName());

  this->Script("pack forget %s",
    this->TimeLineContainer->GetWidgetName());

  this->Script("pack forget %s",
    this->Image->GetWidgetName());
  
  this->Script("pack forget %s",
    this->Label->GetWidgetName());

  this->Script("pack forget %s",
    this->GetWidgetName()); 
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetImageType(int type)
{
  if (this->IsCreated())
    {
    switch (type)
      {
    case vtkPVAnimationCue::NONE:
      break;
    case vtkPVAnimationCue::IMAGE_OPEN:
      this->Image->SetImageOption(
        image_open,
        image_open_width,
        image_open_height,
        image_open_pixel_size,
        image_open_buffer_length);
      break;
    case vtkPVAnimationCue::IMAGE_CLOSE:
      this->Image->SetImageOption(
        image_close,
        image_close_width,
        image_close_height,
        image_close_pixel_size,
        image_close_buffer_length);
      break;
    default:
      vtkErrorMacro("Invalid image type " << type);
      return;
      }
    }
  this->ImageType = type;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetTimeBounds(double bounds[2], int enable_scaling)
{
  this->TimeLine->MoveStartToParameter(bounds[0], enable_scaling);
  this->TimeLine->MoveEndToParameter(bounds[1], enable_scaling);
}

//-----------------------------------------------------------------------------
int vtkPVAnimationCue::GetTimeBounds(double * bounds)
{
  return this->TimeLine->GetParameterBounds(bounds);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::GetFocus()
{
  if (!this->Focus)
    {
    this->GetSelfFocus();
    }
  this->AddTraceEntry("$kw(%s) GetFocus", this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::GetSelfFocus()
{
  this->Focus = 1;
  this->TimeLine->GetFocus();
  // TODO: change color
  vtkKWTkUtilities::ChangeFontWeightToBold(
    this->GetApplication()->GetMainInterp(), this->Label->GetWidgetName());
  this->InvokeEvent(vtkKWEvent::FocusInEvent);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::RemoveFocus()
{
  if (this->Focus)
    {
    this->RemoveSelfFocus();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::RemoveSelfFocus()
{
  this->Focus = 0;
  this->TimeLine->RemoveFocus();
  // TODO: change color
  vtkKWTkUtilities::ChangeFontWeightToNormal(
    this->GetApplication()->GetMainInterp(), this->Label->GetWidgetName());
  this->InvokeEvent(vtkKWEvent::FocusOutEvent);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetAnimatedProxy(vtkSMProxy *proxy)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue does not have any actual proxies associated with it.");
    return;
    }
  this->CueProxy->SetAnimatedProxy(proxy);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetAnimatedPropertyName(const char* name)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue does not have any actual proxies associated with it.");
    return;
    }
  this->CueProxy->SetAnimatedPropertyName(name);
  if (!this->PropertyStatusManager)
    {
    this->PropertyStatusManager = vtkSMPropertyStatusManager::New();
    }
  this->PropertyStatusManager->UnregisterAllProperties();
  this->PropertyStatusManager->RegisterProperty(
    vtkSMVectorProperty::SafeDownCast(this->CueProxy->GetAnimatedProperty()));
  this->PropertyStatusManager->InitializeStatus();
}

//-----------------------------------------------------------------------------
const char* vtkPVAnimationCue::GetAnimatedPropertyName()
{
  if (this->Virtual)
    {
    return NULL;
    }
  return this->CueProxy->GetAnimatedPropertyName();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetAnimatedDomainName(const char* name)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue does not have any actual proxies associated with it.");
    return;
    }
  this->CueProxy->SetAnimatedDomainName(name);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SetAnimatedElement(int index)
{
  if (this->Virtual)
    {
    vtkErrorMacro("Cue does not have any actual proxies associated with it.");
    return;
    }
  this->CueProxy->SetAnimatedElement(index);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::InitializeStatus()
{
  if (this->PropertyStatusManager)
    {
    this->PropertyStatusManager->InitializeStatus();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::KeyFramePropertyChanges(double ntime, int onlyFocus)
{
  if (this->Virtual || !this->PropertyStatusManager || 
    (onlyFocus && !this->HasFocus()))
    {
    return;
    }

  vtkSMProperty* property = this->CueProxy->GetAnimatedProperty();
  int index = this->CueProxy->GetAnimatedElement();
  
  if (!this->PropertyStatusManager->HasPropertyChanged(
      vtkSMVectorProperty::SafeDownCast(property), index ))
    {
    return;
    }
  // animated property has changed.
  // add a keyframe at ntime.
  int id = this->AddNewKeyFrame(ntime);
  if (id == -1)
    {
    vtkErrorMacro("Failed to add new key frame");
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::RemoveAllKeyFrames()
{
  // Don;t directly remove the keyframes...instead pretend that 
  // the timeline nodes are being deleted.
  this->TimeLine->RemoveAll();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->TimeLineParent);
  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->Image);
  this->PropagateEnableState(this->Frame);
  this->PropagateEnableState(this->TimeLineContainer);
  this->PropagateEnableState(this->TimeLineFrame);
  this->PropagateEnableState(this->TimeLine);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::SaveState(ofstream* file)
{
  // All the properties are automatically set except the keyframes
  // So, save the keyframe states. Also, keyframes are saved only when they
  // are non-virtual.
  if (this->Focus)
    {
    *file << "$kw(" << this->GetTclName() << ") GetFocus" << endl;
    }
  if (this->Virtual)
    {
    return;
    }

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(
    this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 

  vtkCollectionIterator* iter = this->PVKeyFrames->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVKeyFrame* pvKF = vtkPVKeyFrame::SafeDownCast(iter->GetCurrentObject());
    *file << endl; 
    *file << "set tempid [$kw(" << this->GetTclName() << ") CreateAndAddKeyFrame " << pvKF->GetKeyTime()
      << " " << pvAM->GetKeyFrameType(pvKF) << "]" << endl;
    *file << "set kw(" << pvKF->GetTclName() << ") [$kw(" << this->GetTclName() <<
      ") GetKeyFrame $tempid ]" << endl;
    pvKF->SaveState(file);
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: " << ((this->Name) ? this->Name : "NULL") << endl;
  os << indent << "LabelText: " << 
    ((this->LabelText)? this->LabelText : "NULL") << endl;
  os << indent << "ImageType: " << this->ImageType << endl;
  os << indent << "ShowTimeLine: " << this->ShowTimeLine << endl;
  os << indent << "Focus: " << this->Focus << endl;
  os << indent << "Virtual: " << this->Virtual << endl;
  os << indent << "ProxiesRegistered: " << this->ProxiesRegistered << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "PointParameters: " << this->PointParameters[0] <<
    ", " << this->PointParameters[1] << endl;
  os << indent << "CueProxyName: " << 
    ((this->CueProxyName)? this->CueProxyName : "NULL") << endl;
  os << indent << "CueProxy: " << this->CueProxy << endl;
  os << indent << "KeyFrameManipulatorProxyName: " <<
    ((this->KeyFrameManipulatorProxyName)? 
     this->KeyFrameManipulatorProxyName : "NULL") << endl;
  os << indent << "KeyFrameManipulatorProxy: " << 
    this->KeyFrameManipulatorProxy << endl;
  os << indent << "PVAnimationScene: " << this->PVAnimationScene << endl;
  os << indent << "PVSource: " << this->PVSource << endl;
  os << indent << "TimeLine: " << this->TimeLine << endl;
}
