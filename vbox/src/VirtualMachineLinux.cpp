#include <stdlib.h>
#include <nsXPCOM.h>
#include <nsIMemory.h>
#include <nsIServiceManager.h>
#include <nsIEventQueue.h>
#include <nsEventQueueUtils.h>
#include "VirtualMachineLinux.h"
#include "ApplicationLog.h"

CVirtualMachineLinux::CVirtualMachineLinux(IMachine *xpcom_machine,
                                           ISession* session) {
  m_internal_machine = xpcom_machine;
  PRUnichar *name, *id;
  xpcom_machine->GetName(&name);
  xpcom_machine->GetId(&id);

  m_name = QString::fromUtf16(name);
  m_iid = QString::fromUtf16(id);

  nsresult rc = xpcom_machine->GetState(&m_state);

  if (NS_FAILED(rc))
    CApplicationLog::Instance()->LogError("Can't get vm state. rc : %x", rc);
  m_session = session;
}
////////////////////////////////////////////////////////////////////////////

CVirtualMachineLinux::~CVirtualMachineLinux() {
  if (m_internal_machine != nsnull) {
    m_internal_machine->Release();
    m_internal_machine = nsnull;
  }
}
////////////////////////////////////////////////////////////////////////////

nsresult
CVirtualMachineLinux::launch_vm(vb_launch_mode_t mode,
                                         IProgress **progress)
{  
  nsresult rc = m_internal_machine->LaunchVMProcess(m_session,
                                                    QString(CVBoxCommons::VM_launch_mode_to_str(mode)).utf16(),
                                                    QString("").utf16(),
                                                    progress);
  CApplicationLog::Instance()->LogTrace("launch_vm result : %x", rc);
  return rc;
}
////////////////////////////////////////////////////////////////////////////

nsresult
CVirtualMachineLinux::save_state(IProgress **progress) {
  return m_internal_machine->SaveState(progress);
}
////////////////////////////////////////////////////////////////////////////

nsresult
CVirtualMachineLinux::pause() {
  nsresult rc;
  rc = m_internal_machine->LockMachine(m_session, LockType_Shared);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<IConsole> console;
  rc = m_session->GetConsole(getter_AddRefs(console));
  if (NS_FAILED(rc)) return rc;
  rc = console->Pause();
  if (NS_FAILED(rc)) return rc;
  rc = m_session->UnlockMachine();
  return rc;
}
////////////////////////////////////////////////////////////////////////////

nsresult
CVirtualMachineLinux::resume() {
  nsresult rc;
  rc = m_internal_machine->LockMachine(m_session, LockType_Shared);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<IConsole> console;
  rc = m_session->GetConsole(getter_AddRefs(console));
  if (NS_FAILED(rc)) return rc;
  rc = console->Resume();
  if (NS_FAILED(rc)) return rc;
  rc = m_session->UnlockMachine();
  return rc;
}
////////////////////////////////////////////////////////////////////////////

nsresult
CVirtualMachineLinux::turn_off(IProgress **progress) {
  nsresult rc = m_internal_machine->LockMachine(m_session, LockType_Shared);
  nsCOMPtr<IConsole> console;
  rc = m_session->GetConsole(getter_AddRefs(console));
  if (NS_FAILED(rc)) return rc;
  rc = console->PowerDown(progress);
  if (NS_FAILED(rc)) return rc;
  rc = m_session->UnlockMachine();
  return rc;
}
////////////////////////////////////////////////////////////////////////////

nsresult
CVirtualMachineLinux::remove(IProgress **progress) {
  UNUSED_ARG(progress);
  nsresult rc;
  IMedium **aMedia;
  PRUint32 cMedia;
  rc = m_internal_machine->Unregister((CleanupMode_T)CleanupMode_Full,  //DetachAllReturnHardDisksOnly,
                                      &cMedia, &aMedia);
  if (NS_FAILED(rc)) return rc;
  //// Delete after fixing Removing vm files on MAC!//////////////////////////
#ifndef RT_OS_DARWIN
  rc = m_internal_machine->DeleteConfig(cMedia, aMedia, progress);
  //          DeleteConfig(cMedia, aMedia, getter_AddRefs(progress));
#endif
  return rc;
}
////////////////////////////////////////////////////////////////////////////

nsresult
CVirtualMachineLinux::run_process(const char *path,
                                           const char *user,
                                           const char *password,
                                           int argc,
                                           const char **argv) {

  nsresult rc = m_internal_machine->LockMachine(m_session, LockType_Shared);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<IConsole> console;
  rc = m_session->GetConsole(getter_AddRefs(console));
  if (NS_FAILED(rc)) return rc;

  nsCOMPtr<IGuest> guest;
  rc = console->GetGuest(getter_AddRefs(guest));
  if (NS_FAILED(rc)) return rc;

  nsCOMPtr<IGuestSession> gsess;

  rc = guest->CreateSession(QString(user).utf16(),
                            QString(password).utf16(),
                            QString("").utf16(), //domain is "". todo add param
                            NULL,
                            getter_AddRefs(gsess));
  if (NS_FAILED(rc)) return rc;

  uint32_t reason = 0;
  rc = gsess->WaitFor(GuestSessionWaitForFlag_Start, 10*1000, &reason);
  if (NS_FAILED(rc)) return rc;
  PRUnichar** env_changes;
  uint32_t env_count = 0;
  rc = gsess->GetEnvironmentChanges(&env_count, &env_changes);
  if (NS_FAILED(rc)) return rc;

  nsCOMPtr<IGuestProcess> gproc = nsnull;

  const PRUnichar** launch_args = new const PRUnichar*[argc];
  for (int i = 0; i < argc; ++i) {
    launch_args[i] = QString(argv[i]).utf16();
  }

  uint32_t flags[] = {ProcessCreateFlag_WaitForProcessStartOnly};
  rc = gsess->ProcessCreate(QString(path).utf16(),
                            argc,
                            launch_args,
                            env_count,
                            (const PRUnichar**)env_changes,
                            sizeof(flags) / sizeof(uint32_t),
                            flags,
                            5000,
                            getter_AddRefs(gproc));
  if (NS_FAILED(rc)) return rc;
  rc = gsess->Close();
  return rc;
}
////////////////////////////////////////////////////////////////////////////
