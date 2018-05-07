#ifndef DLGABOUT_H
#define DLGABOUT_H

#include <QDialog>
#include <map>

#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QThread>

namespace Ui {
  class DlgAbout;
}

class DlgAboutInitializer : public QObject {
  Q_OBJECT
public:
    DlgAboutInitializer(QObject *parent = nullptr) :  QObject(parent){}
  static const int COMPONENTS_COUNT = 13;
public:
  void do_initialization();
  void abort();
  void startWork(){
        QThread* th = new QThread();
        connect(th, &QThread::started,
                this, &DlgAboutInitializer::do_initialization);
        connect(this, &DlgAboutInitializer::finished,
                th, &QThread::quit);
        connect(th, &QThread::finished,
                this, &DlgAboutInitializer::deleteLater);
        connect(th, &QThread::finished,
                th, &QThread::deleteLater);
        this->moveToThread(th);
        th->start();
  }
signals:
  void init_progress(int part, int total);
  void finished();
  void got_p2p_version(QString version);
  void got_chrome_version(QString version);
  void got_rh_version(QString version);
  void got_rh_management_version(QString version);
  void got_x2go_version(QString version);
  void got_vagrant_version(QString version);
  void got_oracle_virtualbox_version(QString version);
  void got_e2e_version(QString version);
  void update_available(QString component_id, bool available);
};
////////////////////////////////////////////////////////////////////////////

/*!
 * \brief This class manages behavior of "About" dialog
 */
class DlgAbout : public QDialog
{
  Q_OBJECT

public:
  explicit DlgAbout(QWidget *parent = 0);
  ~DlgAbout();

private:
  Ui::DlgAbout *ui;

  /*!
   * \brief This struct is used for managing UI elements: progress bar, button, label with version
   */
  struct progress_item_t {
    QLabel* lbl;
    QProgressBar* pb;
    QPushButton* btn;
    QString (*pf_version)();
  };
  std::map<QString, progress_item_t> m_dct_fpb;

  void check_for_versions_and_updates();
  //hide some soft
  void set_visible_chrome(bool value);
  void set_visible_firefox(bool value);
  QString current_browser;

private slots:
  void btn_tray_update_released();
  void btn_p2p_update_released();
  void btn_rh_update_released();
  void btn_rhm_update_released();
  void btn_x2go_update_released();
  void btn_vagrant_update_released();
  void btn_oracle_virtualbox_update_released();
  void btn_chrome_update_release();
  void btn_e2e_update_released();
  void btn_recheck_released();

  void download_progress(const QString &file_id, qint64 rec, qint64 total);
  void update_available(const QString &file_id);
 // void unavailable(const QString &file_id);
  void update_finished(const QString &file_id, bool success);  
  void install_finished(const QString &file_id, bool success);

  void initialization_finished();
  void init_progress_sl(int part, int total);
  void got_p2p_version_sl(QString version);
  void got_chrome_version_sl(QString version);
  void got_rh_version_sl(QString version);
  void got_rh_management_version_sl(QString version);
  void got_x2go_version_sl(QString version);
  void got_vagrant_version_sl(QString version);
  void got_oracle_virtualbox_version_sl(QString version);
  void got_e2e_version_sl(QString version);
  void update_available_sl(const QString &component_id, bool available);
};

#endif // DLGABOUT_H
