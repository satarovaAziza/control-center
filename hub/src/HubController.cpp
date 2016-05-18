#include <QDir>
#include <QThread>
#include <QApplication>

#include "RestWorker.h"
#include "HubController.h"
#include "NotifiactionObserver.h"
#include "SystemCallWrapper.h"
#include "ApplicationLog.h"
#include "SettingsManager.h"
#define UNDEFINED_BALANCE "Undefined balance"

CHubController::CHubController() :
  m_lst_environments(),
  m_lst_resource_hosts(),
  m_balance(UNDEFINED_BALANCE)
{
}

CHubController::~CHubController() {
}
////////////////////////////////////////////////////////////////////////////

int CHubController::refresh_balance() {
  int http_code, err_code, network_error;
  CSSBalance balance = CRestWorker::Instance()->get_balance(http_code, err_code, network_error);
  m_balance = err_code ? QString(UNDEFINED_BALANCE) : QString("Balance: $%1").arg(balance.value());
  return 0;
}
////////////////////////////////////////////////////////////////////////////

int CHubController::refresh_environments() {
  SynchroPrimitives::Locker lock(&m_refresh_cs);
  int http_code, err_code, network_error;
  std::vector<CSSEnvironment> res = CRestWorker::Instance()->get_environments(http_code, err_code, network_error);

  if (err_code) {
    QString err_msg = QString("Refresh environments error : %1").
                      arg(CRestWorker::login_err_to_str((error_login_t)err_code));
    CNotifiactionObserver::NotifyAboutError(err_msg);
    CApplicationLog::Instance()->LogError("Refresh environments error : %d", err_code);
    return 1;
  }

  if (network_error != 0) {
    QString err_msg = QString("Refresh environments network error : %1").arg(network_error);
    CNotifiactionObserver::NotifyAboutError(err_msg);
    CApplicationLog::Instance()->LogError(err_msg.toStdString().c_str());
    return 1;
  }

  if (res == m_lst_environments)
    return 1;
  m_lst_environments = res; //todo std::move
  return 0;
}
////////////////////////////////////////////////////////////////////////////

void CHubController::refresh_containers() {
  SynchroPrimitives::Locker lock(&m_refresh_cs);
  int http_code, err_code, network_error;
  std::vector<CRHInfo> res = CRestWorker::Instance()->get_ssh_containers(http_code, err_code, network_error);

  if (err_code) {
    QString err_msg = QString("Refresh containers info error : %1").
                      arg(CRestWorker::login_err_to_str((error_login_t)err_code));
    CNotifiactionObserver::NotifyAboutError(err_msg);
    CApplicationLog::Instance()->LogError(err_msg.toStdString().c_str());
    return;
  }

  if (network_error != 0) {
    QString err_msg = QString("Refresh containers network error : %1").arg(network_error);
    CNotifiactionObserver::NotifyAboutError(err_msg);
    CApplicationLog::Instance()->LogError(err_msg.toStdString().c_str());
    return;
  }

  if (res == m_lst_resource_hosts)
    return;
  m_lst_resource_hosts = res;
}
////////////////////////////////////////////////////////////////////////////

int
CHubController::ssh_to_container(const CSSEnvironment *env,
                                 const CHubContainer *cont) {
  CApplicationLog::Instance()->LogTrace("ssh to container : %s, env : %s",
                                        cont->name().toStdString().c_str(),
                                        env->name().toStdString().c_str());
  SynchroPrimitives::Locker lock(&m_refresh_cs);
  CApplicationLog::Instance()->LogTrace("ssh_to_container0 locker created");
  system_call_wrapper_error_t err = CSystemCallWrapper::join_to_p2p_swarm(env->hash().toStdString().c_str(),
                                                                          env->key().toStdString().c_str(),
                                                                          "dhcp");
  CApplicationLog::Instance()->LogTrace("join to p2p swarm finished. result : %d", (int)err);
  if (err != SCWE_SUCCESS) {
    QString err_msg = QString("Failed to join to p2p network. Error : %1").
                      arg(CSystemCallWrapper::scwe_error_to_str(err));
    CNotifiactionObserver::NotifyAboutError(err_msg);
    CApplicationLog::Instance()->LogError(err_msg.toStdString().c_str());
    return SLE_JOIN_TO_SWARM_FAILED;
  }

  for (auto i = m_lst_resource_hosts.begin(); i != m_lst_resource_hosts.end(); ++i) {
    for (auto j = i->lst_containers().begin(); j != i->lst_containers().end(); ++j) {
      if (j->id() == cont->id()) {

        static const int MAX_ATTEMTS_COUNT = 5;
        CApplicationLog::Instance()->LogTrace("Check_container_state");
        for (int ac = 0; ac < MAX_ATTEMTS_COUNT; ++ac) {
          err = CSystemCallWrapper::check_container_state(env->hash().toStdString().c_str(),
                                                          i->rh_ip().toStdString().c_str());
          if (err == SCWE_SUCCESS) break;
          QThread::currentThread()->sleep(1);
        }
        CApplicationLog::Instance()->LogTrace("Check_container_state end");
        if (err != SCWE_SUCCESS) return err;

        QFile key_file_pub(QApplication::applicationDirPath() + QDir::separator() +
                           current_user() +
                           QString(".pub"));
        QFile key_file_private(QApplication::applicationDirPath() + QDir::separator() +
                               current_user());
        std::string key;

        if (key_file_pub.exists() &&  key_file_private.exists()) {
          key = (QApplication::applicationDirPath() + QDir::separator() +
                current_user()).toStdString();
        }

        err = CSystemCallWrapper::run_ssh_in_terminal(CSettingsManager::Instance().ssh_user().toStdString().c_str(),
                                                      i->rh_ip().toStdString().c_str(),
                                                      cont->port().toStdString().c_str(),
                                                      key.empty() ? NULL : key.c_str());
        if (err == SCWE_SUCCESS)
          return SLE_SUCCESS;

        QString err_msg = QString("Run SSH failed. Error code : %1").
                          arg(CSystemCallWrapper::scwe_error_to_str(err));
        CNotifiactionObserver::NotifyAboutError(err_msg);
        CApplicationLog::Instance()->LogError(err_msg.toStdString().c_str());
        return SLE_SYSTEM_CALL_FAILED;
      }
    }
  }
  return SLE_CONT_NOT_FOUND;
}
////////////////////////////////////////////////////////////////////////////

int CHubController::ssh_to_container_str(const QString &env_id,
                                         const QString &cont_id) {
  SynchroPrimitives::Locker lock(&m_refresh_cs);
  CSSEnvironment *env = NULL;
  for (auto i = m_lst_environments.begin(); i != m_lst_environments.end(); ++i) {
    if (i->id() != env_id) continue;
    env = &(*i); break;
  }
  if (env == NULL) {return SLE_ENV_NOT_FOUND;}

  const CHubContainer *cont = NULL;
  for (auto j = env->containers().begin(); j != env->containers().end(); ++j) {
    if (j->id() != cont_id) continue;
    cont = &(*j); break;
  }
  if (cont == NULL) { return SLE_CONT_NOT_FOUND;}
  return ssh_to_container(env, cont);
}
////////////////////////////////////////////////////////////////////////////

const QString &CHubController::ssh_launch_err_to_str(int err) {
  static QString lst_err_str[] = {
    "Success", "Environment not found", "Container not found", "Join to p2p swarm failed", "System call failed" };
  return lst_err_str[err%SLE_LAST_ERR];
}
////////////////////////////////////////////////////////////////////////////