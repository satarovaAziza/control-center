#include <QApplication>
#include <QDir>

#include "updater/HubComponentsUpdater.h"
#include "updater/ExecutableUpdater.h"
#include "DownloadFileManager.h"
#include "SystemCallWrapper.h"
#include "RestWorker.h"
#include "NotificationObserver.h"
#include "DownloadFileManager.h"
#include "updater/UpdaterComponentP2P.h"
#include "updater/UpdaterComponentTray.h"
#include "updater/UpdaterComponentRH.h"
#include "updater/UpdaterComponentRHManagement.h"
#include "updater/IUpdaterComponent.h"


using namespace update_system;

CHubComponentsUpdater::CHubComponentsUpdater() {
  IUpdaterComponent *uc_tray, *uc_p2p, *uc_rh, *uc_rhm, *uc_x2go, *uc_vagrant, *uc_oracle_virtualbox;
  uc_tray = new CUpdaterComponentTray;
  uc_p2p  = new CUpdaterComponentP2P;
  uc_rh   = new CUpdaterComponentRH;
  uc_rhm  = new CUpdaterComponentRHM;
  uc_x2go = new CUpdaterComponentX2GO;
  uc_vagrant = new CUpdaterComponentVAGRANT;
  uc_oracle_virtualbox = new CUpdaterComponentORACLE_VIRTUALBOX;
  IUpdaterComponent* ucs[] = {uc_tray, uc_p2p, uc_rh, uc_rhm, uc_x2go, uc_vagrant, uc_oracle_virtualbox, NULL};

  m_dct_components[IUpdaterComponent::TRAY] = CUpdaterComponentItem(uc_tray);
  m_dct_components[IUpdaterComponent::P2P]  = CUpdaterComponentItem(uc_p2p);
  m_dct_components[IUpdaterComponent::RH]   = CUpdaterComponentItem(uc_rh);
  m_dct_components[IUpdaterComponent::RHMANAGEMENT] = CUpdaterComponentItem(uc_rhm);
  m_dct_components[IUpdaterComponent::X2GO] = CUpdaterComponentItem(uc_x2go);
  m_dct_components[IUpdaterComponent::VAGRANT] = CUpdaterComponentItem(uc_vagrant);
  m_dct_components[IUpdaterComponent::ORACLE_VIRTUALBOX] = CUpdaterComponentItem(uc_oracle_virtualbox);

  for(int i = 0; ucs[i] ;++i) {
    connect(&m_dct_components[ucs[i]->component_id()], &CUpdaterComponentItem::timer_timeout,
        this, &CHubComponentsUpdater::update_component_timer_timeout);
    connect(ucs[i], &IUpdaterComponent::update_progress,
        this, &CHubComponentsUpdater::update_component_progress_sl);
    connect(ucs[i], &IUpdaterComponent::update_finished,
        this, &CHubComponentsUpdater::update_component_finished_sl);
    connect(ucs[i], &IUpdaterComponent::install_finished,
            this, &CHubComponentsUpdater::install_component_finished_sl);
  }
  ///
  set_p2p_update_freq();
  set_rh_update_freq();
  set_tray_update_freq();
  set_rh_management_update_freq();
}

CHubComponentsUpdater::~CHubComponentsUpdater() {
  for(auto i = m_dct_components.begin(); i != m_dct_components.end(); ++i) {
    delete i->second.Component();
  }
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_update_freq(const QString &component_id,
                                       CSettingsManager::update_freq_t freq) {
  if (m_dct_components.find(component_id) == m_dct_components.end()) {
    qCritical(
          "can't find component updater in map with id = %s", component_id.toStdString().c_str());
    return;
  }

  m_dct_components[component_id].timer_stop();
  if (freq == CSettingsManager::UF_NEVER)
    return;

  m_dct_components[component_id].set_timer_interval(
        CSettingsManager::update_freq_to_sec(freq) * 1000);
  m_dct_components[component_id].timer_start();
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_component_autoupdate(const QString &component_id,
                                                bool autoupdate) {
  if (m_dct_components.find(component_id) == m_dct_components.end()) {
    qCritical(
          "can't find component updater in map with id = %s", component_id.toStdString().c_str());
    return;
  }
  m_dct_components[component_id].autoupdate = autoupdate;
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::update_component_timer_timeout(const QString &component_id) {
  if (m_dct_components.find(component_id) == m_dct_components.end()) {
    qCritical(
          "can't find component updater in map with id = %s", component_id.toStdString().c_str());
    return;
  }

  m_dct_components[component_id].timer_stop();
  if (m_dct_components[component_id].Component()->update_available()) {
    if (m_dct_components[component_id].autoupdate) {
      CNotificationObserver::Instance()->Info(
            tr("%1 updating started").arg(IUpdaterComponent::component_id_to_user_view(component_id)),  DlgNotification::N_NO_ACTION);
      m_dct_components[component_id].Component()->update();
    } else {

      CNotificationObserver::Instance()->Info(
            tr("New version of %1 is available!")
                  .arg(IUpdaterComponent::component_id_to_user_view(component_id)),
                  IUpdaterComponent::component_id_to_notification_action(component_id));
    }
  }
  m_dct_components[component_id].timer_start();
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_p2p_update_freq() {
  set_update_freq(IUpdaterComponent::P2P, CSettingsManager::Instance().p2p_update_freq());
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_rh_update_freq() {
  set_update_freq(IUpdaterComponent::RH, CSettingsManager::Instance().rh_update_freq());
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_tray_update_freq() {
  set_update_freq(IUpdaterComponent::TRAY, CSettingsManager::Instance().tray_update_freq());
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_rh_management_update_freq() {
  set_update_freq(IUpdaterComponent::RHMANAGEMENT,
                  CSettingsManager::Instance().rh_management_update_freq());
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_p2p_autoupdate() {
  set_component_autoupdate(IUpdaterComponent::P2P,
                           CSettingsManager::Instance().p2p_autoupdate());
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_rh_autoupdate() {
  set_component_autoupdate(IUpdaterComponent::RH,
                           CSettingsManager::Instance().rh_autoupdate());
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_tray_autoupdate() {
  set_component_autoupdate(IUpdaterComponent::TRAY,
                           CSettingsManager::Instance().tray_autoupdate());
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::set_rh_management_autoupdate() {
  set_component_autoupdate(IUpdaterComponent::RHMANAGEMENT,
                           CSettingsManager::Instance().rh_management_autoupdate());
}
////////////////////////////////////////////////////////////////////////////

bool
CHubComponentsUpdater::is_update_available(const QString &component_id) {
  if (m_dct_components.find(component_id) == m_dct_components.end()) {
    qCritical(
          "can't find component updater in map with id = %s", component_id.toStdString().c_str());
    return false;
  }
  return m_dct_components[component_id].Component()->update_available();
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::force_update(const QString &component_id) {
  if (m_dct_components.find(component_id) == m_dct_components.end()) {
    qCritical(
          "can't find component updater in map with id = %s", component_id.toStdString().c_str());
    return;
  }
  m_dct_components[component_id].Component()->update();
}
void CHubComponentsUpdater::force_update_p2p() {
  force_update(IUpdaterComponent::P2P);
}
void CHubComponentsUpdater::force_update_tray() {
  force_update(IUpdaterComponent::TRAY);
}
void CHubComponentsUpdater::force_update_rh() {
  force_update(IUpdaterComponent::RH);
}
void CHubComponentsUpdater::force_update_rhm() {
  force_update(IUpdaterComponent::RHMANAGEMENT);
}


////////////////////////////////////////////////////////////////////////////

void CHubComponentsUpdater::install(const QString &component_id){
    qDebug()
        <<"Install"<<component_id.toStdString().c_str()<<"called";
    if (m_dct_components.find(component_id) == m_dct_components.end()) {
      qCritical(
            "can't find component updater in map with id = %s", component_id.toStdString().c_str());
      return;
    }
    emit install_component_started(IUpdaterComponent::component_id_to_user_view(component_id));
    m_dct_components[component_id].Component()->install();
}

void CHubComponentsUpdater::install_p2p(){
    install(IUpdaterComponent::P2P);
}

void CHubComponentsUpdater::install_x2go(){
    install(IUpdaterComponent::X2GO);
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::update_component_progress_sl(const QString& file_id, qint64 cur,
                                                    qint64 full) {
  emit download_file_progress(file_id, cur, full);
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::update_component_finished_sl(const QString& file_id, bool replaced) {
  emit updating_finished(file_id, replaced);
}
////////////////////////////////////////////////////////////////////////////
void CHubComponentsUpdater::install_component_finished_sl(const QString &file_id, bool replaced){
    emit installing_finished(file_id, replaced);
}
/////////////////////////////////////////////////////////////////////////////
QString CHubComponentsUpdater::component_name(const QString &component_id){
    if(m_dct_components.find(component_id) == m_dct_components.end())
        return "";
    return m_dct_components[component_id].Component()->component_id_to_user_view(component_id);
}

/////////////////////////////////////////////////////////////////////////////
bool CHubComponentsUpdater::is_in_progress(const QString &component_id){
    if(m_dct_components.find(component_id) == m_dct_components.end())
        return true;
    else return m_dct_components[component_id].Component()->is_in_progress();
}
///// experimentation permutation
void SilentPackageInstallerVAGRANT::startWork(){
    QThread* thread = new QThread();
    connect(thread, &QThread::started,
            this, &SilentPackageInstallerVAGRANT::execute_remote_command);
    connect(this, &SilentPackageInstallerVAGRANT::outputReceived,
            thread, &QThread::quit);
    connect(thread, &QThread::finished,
            this, &SilentPackageInstallerVAGRANT::deleteLater);
    connect(thread, &QThread::finished,
            thread, &QThread::deleteLater);
    this->moveToThread(thread);
    thread->start();
}

