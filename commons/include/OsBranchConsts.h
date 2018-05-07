#ifndef OSBRANCHCONSTS_H
#define OSBRANCHCONSTS_H

#include <QString>
#include <vector>

enum branch_t {
  BT_MASTER = 0,
  BT_PROD = 1,
  BT_DEV = 2
};

enum os_t {
  OS_LINUX = 0,
  OS_WIN = 1,
  OS_MAC = 2,
  OS_MAC_LIN = 3
};

enum browser_t {
    BR_CHROME = 0,
    BR_SAFARI,
    BR_FIREFOX,
    BR_EDGE
};

/**
 * @brief branch_t enum to type struct for template specification
 */
template<branch_t v> struct Branch2Type {
  enum {val = v};
};

/**
 * @brief os_t enum to type struct for template specification
 */
template<os_t v> struct Os2Type {
  enum {val = v};
};

/**
 * @brief int to type struct for template specification
 */
template<int v> struct Int2Type {
  enum {val = v};
};

template<browser_t v> struct Browser2Type
{
    enum {val = v};
};
////////////////////////////////////////////////////////////////////////////

const QString& tray_kurjun_file_name();
const QString& p2p_kurjun_file_name();
const QString& p2p_kurjun_package_name();
const QString& x2go_kurjun_package_name();
const QString& vagrant_kurjun_package_name();
const QString& oracle_virtualbox_kurjun_package_name();
const QString& chrome_kurjun_package_name();
const QString& p2p_dht_arg();
const QString& p2p_rest_url();


const QString& hub_post_url();
const QString& hub_register_url();
const QString& hub_user_profile_url();
const QString& hub_get_url();
const QString& hub_health_url();
const QString& hub_gorjun_url();
const QString& hub_billing_url();

const char ** ssdp_rh_search_target_arr();

const QString& default_p2p_path();
const QString& default_vagrant_path();
const QString& default_terminal();
const QString& default_x2goclient_path();
const QString& default_term_arg();
const QString& default_oracle_virtualbox_path();
const QString& default_default_browser();
const QString& default_chrome_extensions_path();
const QStringList& supported_browsers();

const QString& hub_site();
const QString& ssh_keygen_cmd_path();
const QString& ssh_cmd_path();
const QString& scp_cmd_path();

const QString& which_cmd();
const QString& default_chrome_path();

const QString& subutai_command();
const QString& snap_p2p_path();

const QString& p2p_package_url();
const QString& x2goclient_url();

const QString& current_branch_name();
const QString& branch_name_str();
const QString& base_interface_name();
void current_os_info(std::vector<std::pair<QString, QString> >& v);

const QString& subutai_e2e_id(const QString& current_browser);
#endif // OSBRANCHCONSTS_H
