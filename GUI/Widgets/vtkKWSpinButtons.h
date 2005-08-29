/*=========================================================================

  Module:    vtkKWSpinButtons.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSpinButtons - A set of spin-buttons.
// .SECTION Description
// This widget implements a small set of two buttons that can be used
// to switch to the next or previous value of an external variable through
// callbacks.
// The buttons can be set to display up/down or left/right arrows, and laid
// out vertically or horizontally.
// The 'previous' button is mapped to the up/left arrow, the 'next' button
// is mapped to the 'down/right' arrow.

#ifndef __vtkKWSpinButtons_h
#define __vtkKWSpinButtons_h

#include "vtkKWCompositeWidget.h"

class vtkKWApplication;
class vtkKWPushButton;

class KWWIDGETS_EXPORT vtkKWSpinButtons : public vtkKWCompositeWidget
{
public:
  static vtkKWSpinButtons* New();
  vtkTypeRevisionMacro(vtkKWSpinButtons,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Get the buttons
  vtkGetObjectMacro(PreviousButton, vtkKWPushButton);
  vtkGetObjectMacro(NextButton, vtkKWPushButton);

  // Description:
  // Specifies the commands to associate to the next and previous 
  // buttons.
  virtual void SetPreviousCommand(vtkObject *object, const char *method);
  virtual void SetNextCommand(vtkObject *object, const char *method);

  // Description:
  // Set/Get the arrow orientation of the spin buttons.
  // If set to horizontal, left/right arrows will be used. If set to
  // vertical, up/down arrows will be used.
  //BTX
  enum 
  {
    ArrowOrientationHorizontal = 0,
    ArrowOrientationVertical
  };
  //ETX
  virtual void SetArrowOrientation(int);
  vtkGetMacro(ArrowOrientation, int);
  virtual void SetArrowOrientationToHorizontal()
    { this->SetArrowOrientation(
      vtkKWSpinButtons::ArrowOrientationHorizontal); };
  virtual void SetArrowOrientationToVertical()
    { this->SetArrowOrientation(
      vtkKWSpinButtons::ArrowOrientationVertical); };

  // Description:
  // Set/Get the layout of the spin buttons.
  // If set to horizontal, the 'previous' button is packed to the 
  // left of the 'next' button. If set to vertical, the 'previous' button
  // is packed on top of the 'next' button.
  //BTX
  enum 
  {
    LayoutOrientationHorizontal = 0,
    LayoutOrientationVertical
  };
  //ETX
  virtual void SetLayoutOrientation(int);
  vtkGetMacro(LayoutOrientation, int);
  virtual void SetLayoutOrientationToHorizontal()
    { this->SetLayoutOrientation(
      vtkKWSpinButtons::LayoutOrientationHorizontal); };
  virtual void SetLayoutOrientationToVertical()
    { this->SetLayoutOrientation(
      vtkKWSpinButtons::LayoutOrientationVertical); };

  // Description:
  // Set/Get the padding that will be applied around each buttons.
  // (default to 0).
  virtual void SetButtonsPadX(int);
  vtkGetMacro(ButtonsPadX, int);
  virtual void SetButtonsPadY(int);
  vtkGetMacro(ButtonsPadY, int);

  // Description:
  // Convenience method to set the buttons width.
  // No effects if called before Create()
  virtual void SetButtonsWidth(int w);
  virtual int GetButtonsWidth();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWSpinButtons();
  ~vtkKWSpinButtons();

  vtkKWPushButton *PreviousButton;
  vtkKWPushButton *NextButton;

  int ArrowOrientation;
  int LayoutOrientation;

  int ButtonsPadX;
  int ButtonsPadY;

  virtual void Pack();
  virtual void UpdateArrowOrientation();

private:
  vtkKWSpinButtons(const vtkKWSpinButtons&); // Not implemented
  void operator=(const vtkKWSpinButtons&); // Not implemented
};


#endif



