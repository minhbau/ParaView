#include "vtkNetworkImageWriter.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"

#include <cstring>

vtkStandardNewMacro(vtkNetworkImageWriter);
vtkCxxSetObjectMacro(vtkNetworkImageWriter, Writer, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkNetworkImageWriter, Interpreter, vtkClientServerInterpreter);
//----------------------------------------------------------------------------
vtkNetworkImageWriter::vtkNetworkImageWriter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->SetInterpreter(vtkClientServerInterpreterInitializer::GetGlobalInterpreter());
}

//----------------------------------------------------------------------------
vtkNetworkImageWriter::~vtkNetworkImageWriter()
{
  this->SetWriter(nullptr);
  this->SetInterpreter(nullptr);
}

//-----------------------------------------------------------------------------
void vtkNetworkImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Writer: " << this->Writer << endl;
  os << indent << "OutputDestination: " << this->OutputDestination << endl;
  os << indent << "Interpreter: " << this->Interpreter << endl;
}

//----------------------------------------------------------------------------
int vtkNetworkImageWriter::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return this->Superclass::FillInputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkNetworkImageWriter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto session =
    vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetActiveSession());
  const vtkPVSession::ServerFlags roles = session->GetProcessRoles();

  if (this->OutputDestination == CLIENT)
  {
    if ((roles & vtkPVSession::CLIENT) != 0)
    {
      // client (or builtin)
      auto input = vtkDataObject::GetData(inputVector[0], 0);
      this->WriteLocally(input);
      return 1;
    }
    else
    {
      // on server-rank; nothing to do.
      return 1;
    }
  }
  else if (this->OutputDestination == DATA_SERVER_ROOT)
  {
    if ((roles & vtkPVSession::CLIENT) != 0)
    {
      // client
      auto input = vtkDataObject::GetData(inputVector[0], 0);
      if (auto controller = session->GetController(vtkPVSession::DATA_SERVER_ROOT))
      {
        controller->Send(input, /*remote process id*/ 1, /*tag -- makeup some number*/ 102802);
      }
      else
      {
        // controller is null in built-in mode.
        // not in client-server mode, must be in builtin mode, just write locally.
        this->WriteLocally(input);
      }
      return 1;
    }
    else
    {
      // on server-rank. this can be on any of the data-server ranks or render-server ranks. We
      // only want to write data on data-server-root node.
      if ((roles & vtkPVSession::DATA_SERVER) != 0)
      {
        if (auto controller = session->GetController(vtkPVSession::CLIENT))
        {
          auto data =
            vtkSmartPointer<vtkDataObject>::Take(controller->ReceiveDataObject(1, 102802));
          this->WriteLocally(data);
        }
        else
        {
          // controller is null on satellite ranks when running in parallel. nothing to do on this
          // rank.
        }
      }
      return 1;
    }
  }
  else
  {
    vtkErrorMacro("Unknown output destination: " << this->OutputDestination);
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkNetworkImageWriter::WriteLocally(vtkDataObject* input)
{
  if (this->Writer)
  {
    vtkLogF(TRACE, "Writing file locally using writer %s", vtkLogIdentifier(this->Writer));
    this->Writer->SetInputDataObject(input);
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << this->Writer << "Write"
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    this->Writer->SetInputDataObject(nullptr);
  }
  else
  {
    vtkErrorMacro("No writer specified! Failed to write.");
  }
}
