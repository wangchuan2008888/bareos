/*
   BAREOS® - Backup Archiving REcovery Open Sourced

   Copyright (C) 2000-2010 Free Software Foundation Europe e.V.
   Copyright (C) 2011-2012 Planets Communications B.V.
   Copyright (C) 2013-2018 Bareos GmbH & Co. KG

   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.
*/
/*
 * Kern Sibbald, January MM
 */
#ifndef BAREOS_CORE_SRC_LIB_PARSE_CONF_H_
#define BAREOS_CORE_SRC_LIB_PARSE_CONF_H_


#include "include/bareos.h"
#include "include/bc_types.h"
#include "lib/parse_conf_callbacks.h"
#include "lib/s_password.h"
#include "lib/tls_conf.h"
#include "lib/parse_conf.h"
#include "lib/keyword_table_s.h"

#include <functional>

struct ResourceItem;
class CommonResourceHeader;
class ConfigurationParser;

enum parse_state
{
  p_none,
  p_resource
};

/**
 * Common TLS-Settings for both (Certificate and PSK).
 */
#define TLS_COMMON_CONFIG(res)                                                 \
  {"TlsAuthenticate",                                                          \
   CFG_TYPE_BOOL,                                                              \
   ITEM(res.authenticate_),                                                    \
   0,                                                                          \
   CFG_ITEM_DEFAULT,                                                           \
   "false",                                                                    \
   NULL,                                                                       \
   "Use TLS only to authenticate, not for encryption."},                       \
      {"TlsEnable", CFG_TYPE_BOOL, ITEM(res.tls_enable_), 0, CFG_ITEM_DEFAULT, \
       "true",      NULL,          "Enable TLS support."},                     \
      {"TlsRequire",                                                           \
       CFG_TYPE_BOOL,                                                          \
       ITEM(res.tls_require_),                                                 \
       0,                                                                      \
       CFG_ITEM_DEFAULT,                                                       \
       "false",                                                                \
       NULL,                                                                   \
       "Without setting this to yes, Bareos can fall back to use unencrypted " \
       "connections. "                                                         \
       "Enabling this implicitly sets \"TLS Enable = yes\"."},                 \
      {"TlsCipherList",                                                        \
       CFG_TYPE_STDSTRDIR,                                                     \
       ITEM(res.cipherlist_),                                                  \
       0,                                                                      \
       CFG_ITEM_PLATFORM_SPECIFIC,                                             \
       NULL,                                                                   \
       NULL,                                                                   \
       "List of valid TLS Ciphers."},                                          \
  {                                                                            \
    "TlsDhFile", CFG_TYPE_STDSTRDIR, ITEM(res.tls_cert_.dhfile_), 0, 0, NULL,  \
        NULL,                                                                  \
        "Path to PEM encoded Diffie-Hellman parameter file. "                  \
        "If this directive is specified, DH key exchange will be used for "    \
        "the ephemeral keying, "                                               \
        "allowing for forward secrecy of communications."                      \
  }

/*
 * TLS Settings for Certificate only
 */
#define TLS_CERT_CONFIG(res)                                               \
  {"TlsVerifyPeer",                                                        \
   CFG_TYPE_BOOL,                                                          \
   ITEM(res.tls_cert_.verify_peer_),                                       \
   0,                                                                      \
   CFG_ITEM_DEFAULT,                                                       \
   "false",                                                                \
   NULL,                                                                   \
   "If disabled, all certificates signed by a known CA will be accepted. " \
   "If enabled, the CN of a certificate must the Address or in the \"TLS " \
   "Allowed CN\" list."},                                                  \
      {"TlsCaCertificateFile",                                             \
       CFG_TYPE_STDSTRDIR,                                                 \
       ITEM(res.tls_cert_.ca_certfile_),                                   \
       0,                                                                  \
       0,                                                                  \
       NULL,                                                               \
       NULL,                                                               \
       "Path of a PEM encoded TLS CA certificate(s) file."},               \
      {"TlsCaCertificateDir",                                              \
       CFG_TYPE_STDSTRDIR,                                                 \
       ITEM(res.tls_cert_.ca_certdir_),                                    \
       0,                                                                  \
       0,                                                                  \
       NULL,                                                               \
       NULL,                                                               \
       "Path of a TLS CA certificate directory."},                         \
      {"TlsCertificateRevocationList",                                     \
       CFG_TYPE_STDSTRDIR,                                                 \
       ITEM(res.tls_cert_.crlfile_),                                       \
       0,                                                                  \
       0,                                                                  \
       NULL,                                                               \
       NULL,                                                               \
       "Path of a Certificate Revocation List file."},                     \
      {"TlsCertificate",                                                   \
       CFG_TYPE_STDSTRDIR,                                                 \
       ITEM(res.tls_cert_.certfile_),                                      \
       0,                                                                  \
       0,                                                                  \
       NULL,                                                               \
       NULL,                                                               \
       "Path of a PEM encoded TLS certificate."},                          \
      {"TlsKey",                                                           \
       CFG_TYPE_STDSTRDIR,                                                 \
       ITEM(res.tls_cert_.keyfile_),                                       \
       0,                                                                  \
       0,                                                                  \
       NULL,                                                               \
       NULL,                                                               \
       "Path of a PEM encoded private key. It must correspond to the "     \
       "specified \"TLS Certificate\"."},                                  \
  {                                                                        \
    "TlsAllowedCn", CFG_TYPE_ALIST_STR,                                    \
        ITEM(res.tls_cert_.allowed_certificate_common_names_), 0, 0, NULL, \
        NULL, "\"Common Name\"s (CNs) of the allowed peer certificates."   \
  }

/* For storing name_addr items in res_items table */
#define ITEM(x)        \
  {                    \
    (char**)&res_all.x \
  }

/*
 * Master Resource configuration structure definition
 * This is the structure that defines the resources that are available to this
 * daemon.
 */
struct ResourceTable {
  const char* name;    /* Resource name */
  ResourceItem* items; /* List of resource keywords */
  uint32_t rcode;      /* Code if needed */
  uint32_t size;       /* Size of resource */

  std::function<void*(void* res)>
      initres; /* this shoud call the new replacement*/
};

/*
 * Common Resource definitions
 */
#define MAX_RES_NAME_LENGTH \
  MAX_NAME_LENGTH - 1 /* maximum resource name length */

/*
 * Config item flags.
 */
#define CFG_ITEM_REQUIRED 0x1   /* Item required */
#define CFG_ITEM_DEFAULT 0x2    /* Default supplied */
#define CFG_ITEM_NO_EQUALS 0x4  /* Don't scan = after name */
#define CFG_ITEM_DEPRECATED 0x8 /* Deprecated config option */
#define CFG_ITEM_ALIAS 0x10     /* Item is an alias for another */

/*
 * CFG_ITEM_DEFAULT_PLATFORM_SPECIFIC: the value may differ between different
 * platforms (or configure settings). This information is used for the
 * documentation.
 */
#define CFG_ITEM_PLATFORM_SPECIFIC 0x20

enum
{
  /*
   * Standard resource types. handlers in res.c
   */
  CFG_TYPE_STR = 1,           /* String */
  CFG_TYPE_DIR = 2,           /* Directory */
  CFG_TYPE_MD5PASSWORD = 3,   /* MD5 hashed Password */
  CFG_TYPE_CLEARPASSWORD = 4, /* Clear text Password */
  CFG_TYPE_AUTOPASSWORD =
      5,             /* Password stored in clear when needed otherwise hashed */
  CFG_TYPE_NAME = 6, /* Name */
  CFG_TYPE_STRNAME = 7,            /* String Name */
  CFG_TYPE_RES = 8,                /* Resource */
  CFG_TYPE_ALIST_RES = 9,          /* List of resources */
  CFG_TYPE_ALIST_STR = 10,         /* List of strings */
  CFG_TYPE_ALIST_DIR = 11,         /* List of dirs */
  CFG_TYPE_INT16 = 12,             /* 16 bits Integer */
  CFG_TYPE_PINT16 = 13,            /* Positive 16 bits Integer (unsigned) */
  CFG_TYPE_INT32 = 14,             /* 32 bits Integer */
  CFG_TYPE_PINT32 = 15,            /* Positive 32 bits Integer (unsigned) */
  CFG_TYPE_MSGS = 16,              /* Message resource */
  CFG_TYPE_INT64 = 17,             /* 64 bits Integer */
  CFG_TYPE_BIT = 18,               /* Bitfield */
  CFG_TYPE_BOOL = 19,              /* Boolean */
  CFG_TYPE_TIME = 20,              /* Time value */
  CFG_TYPE_SIZE64 = 21,            /* 64 bits file size */
  CFG_TYPE_SIZE32 = 22,            /* 32 bits file size */
  CFG_TYPE_SPEED = 23,             /* Speed limit */
  CFG_TYPE_DEFS = 24,              /* Definition */
  CFG_TYPE_LABEL = 25,             /* Label */
  CFG_TYPE_ADDRESSES = 26,         /* List of ip addresses */
  CFG_TYPE_ADDRESSES_ADDRESS = 27, /* Ip address */
  CFG_TYPE_ADDRESSES_PORT = 28,    /* Ip port */
  CFG_TYPE_PLUGIN_NAMES = 29,      /* Plugin Name(s) */
  CFG_TYPE_STDSTR = 30,            /* String as std::string*/
  CFG_TYPE_STDSTRDIR = 31,         /* Directory as std::string*/

  /*
   * Director resource types. handlers in dird_conf.
   */
  CFG_TYPE_ACL = 50,              /* User Access Control List */
  CFG_TYPE_AUDIT = 51,            /* Auditing Command List */
  CFG_TYPE_AUTHPROTOCOLTYPE = 52, /* Authentication Protocol */
  CFG_TYPE_AUTHTYPE = 53,         /* Authentication Type */
  CFG_TYPE_DEVICE = 54,           /* Device resource */
  CFG_TYPE_JOBTYPE = 55,          /* Type of Job */
  CFG_TYPE_PROTOCOLTYPE = 56,     /* Protocol */
  CFG_TYPE_LEVEL = 57,            /* Backup Level */
  CFG_TYPE_REPLACE = 58,          /* Replace option */
  CFG_TYPE_SHRTRUNSCRIPT = 59,    /* Short Runscript definition */
  CFG_TYPE_RUNSCRIPT = 60,        /* Runscript */
  CFG_TYPE_RUNSCRIPT_CMD = 61,    /* Runscript Command */
  CFG_TYPE_RUNSCRIPT_TARGET = 62, /* Runscript Target (Host) */
  CFG_TYPE_RUNSCRIPT_BOOL = 63,   /* Runscript Boolean */
  CFG_TYPE_RUNSCRIPT_WHEN = 64,   /* Runscript When expression */
  CFG_TYPE_MIGTYPE = 65,          /* Migration Type */
  CFG_TYPE_INCEXC = 66,           /* Include/Exclude item */
  CFG_TYPE_RUN = 67,              /* Schedule Run Command */
  CFG_TYPE_ACTIONONPURGE = 68,    /* Action to perform on Purge */
  CFG_TYPE_POOLTYPE = 69,         /* Pool Type */

  /*
   * Director fileset options. handlers in dird_conf.
   */
  CFG_TYPE_FNAME = 80,      /* Filename */
  CFG_TYPE_PLUGINNAME = 81, /* Pluginname */
  CFG_TYPE_EXCLUDEDIR = 82, /* Exclude directory */
  CFG_TYPE_OPTIONS = 83,    /* Options block */
  CFG_TYPE_OPTION = 84,     /* Option of Options block */
  CFG_TYPE_REGEX = 85,      /* Regular Expression */
  CFG_TYPE_BASE = 86,       /* Basejob Expression */
  CFG_TYPE_WILD = 87,       /* Wildcard Expression */
  CFG_TYPE_PLUGIN = 88,     /* Plugin definition */
  CFG_TYPE_FSTYPE = 89,     /* FileSytem match criterium (UNIX)*/
  CFG_TYPE_DRIVETYPE = 90,  /* DriveType match criterium (Windows) */
  CFG_TYPE_META = 91,       /* Meta tag */

  /*
   * Storage daemon resource types
   */
  CFG_TYPE_DEVTYPE = 201,      /* Device Type */
  CFG_TYPE_MAXBLOCKSIZE = 202, /* Maximum Blocksize */
  CFG_TYPE_IODIRECTION = 203,  /* IO Direction */
  CFG_TYPE_CMPRSALGO = 204,    /* Compression Algorithm */

  /*
   * File daemon resource types
   */
  CFG_TYPE_CIPHER = 301 /* Encryption Cipher */
};

struct DatatypeName {
  const int number;
  const char* name;
  const char* description;
};

typedef void(INIT_RES_HANDLER)(ResourceItem* item, int pass);
typedef void(STORE_RES_HANDLER)(LEX* lc,
                                ResourceItem* item,
                                int index,
                                int pass);
typedef void(PRINT_RES_HANDLER)(ResourceItem* items,
                                int i,
                                PoolMem& cfg_str,
                                bool hide_sensitive_data,
                                bool inherited);

class QualifiedResourceNameTypeConverter;

/*
 * New C++ configuration routines
 */
class ConfigurationParser {
 public:
  std::string cf_;                    /* Config file parameter */
  LEX_ERROR_HANDLER* scan_error_;     /* Error handler if non-null */
  LEX_WARNING_HANDLER* scan_warning_; /* Warning handler if non-null */
  INIT_RES_HANDLER*
      init_res_; /* Init resource handler for non default types if non-null */
  STORE_RES_HANDLER*
      store_res_; /* Store resource handler for non default types if non-null */
  PRINT_RES_HANDLER*
      print_res_; /* Print resource handler for non default types if non-null */

  int32_t err_type_;     /* The way to Terminate on failure */
  void* res_all_;        /* Pointer to res_all buffer */
  int32_t res_all_size_; /* Length of buffer */
  bool omit_defaults_; /* Omit config variables with default values when dumping
                          the config */

  int32_t r_first_;          /* First daemon resource type */
  int32_t r_last_;           /* Last daemon resource type */
  int32_t r_own_;            /* own resource type */
  ResourceTable* resources_; /* Pointer to table of permitted resources */
  CommonResourceHeader** res_head_; /* Pointer to defined resources */
  mutable brwlock_t res_lock_;      /* Resource lock */

  SaveResourceCb_t SaveResourceCb_;
  DumpResourceCb_t DumpResourceCb_;
  FreeResourceCb_t FreeResourceCb_;

  ConfigurationParser();
  ConfigurationParser(const char* cf,
                      LEX_ERROR_HANDLER* ScanError,
                      LEX_WARNING_HANDLER* scan_warning,
                      INIT_RES_HANDLER* init_res,
                      STORE_RES_HANDLER* StoreRes,
                      PRINT_RES_HANDLER* print_res,
                      int32_t err_type,
                      void* vres_all,
                      int32_t res_all_size,
                      int32_t r_first,
                      int32_t r_last,
                      ResourceTable* resources,
                      CommonResourceHeader** res_head,
                      const char* config_default_filename,
                      const char* config_include_dir,
                      void (*ParseConfigReadyCb)(ConfigurationParser&),
                      SaveResourceCb_t SaveResourceCb,
                      DumpResourceCb_t DumpResourceCb,
                      FreeResourceCb_t FreeResourceCb);

  ~ConfigurationParser();

  bool IsUsingConfigIncludeDir() const { return use_config_include_dir_; }
  bool ParseConfig();
  bool ParseConfigFile(const char* cf,
                       void* caller_ctx,
                       LEX_ERROR_HANDLER* ScanError = NULL,
                       LEX_WARNING_HANDLER* scan_warning = NULL);
  const std::string& get_base_config_path() const { return used_config_path_; }
  void FreeResources();
  CommonResourceHeader** save_resources();
  CommonResourceHeader** new_res_head();
  void InitResource(int type,
                    ResourceItem* items,
                    int pass,
                    std::function<void*(void* res)> initres);
  bool RemoveResource(int type, const char* name);
  void DumpResources(void sendit(void* sock, const char* fmt, ...),
                     void* sock,
                     bool hide_sensitive_data = false);
  const char* get_resource_type_name(int code);
  int GetResourceCode(const char* resource_type);
  ResourceTable* get_resource_table(int resource_type);
  ResourceTable* get_resource_table(const char* resource_type_name);
  int GetResourceItemIndex(ResourceItem* res_table, const char* item);
  ResourceItem* get_resource_item(ResourceItem* res_table, const char* item);
  bool GetPathOfResource(PoolMem& path,
                         const char* component,
                         const char* resourcetype,
                         const char* name,
                         bool set_wildcards = false);
  bool GetPathOfNewResource(PoolMem& path,
                            PoolMem& extramsg,
                            const char* component,
                            const char* resourcetype,
                            const char* name,
                            bool error_if_exits = false,
                            bool create_directories = false);
  CommonResourceHeader* GetNextRes(int rcode, CommonResourceHeader* res) const;
  CommonResourceHeader* GetResWithName(int rcode,
                                       const char* name,
                                       bool lock = true) const;
  void b_LockRes(const char* file, int line) const;
  void b_UnlockRes(const char* file, int line) const;
  const char* res_to_str(int rcode) const;
  bool StoreResource(int type,
                     LEX* lc,
                     ResourceItem* item,
                     int index,
                     int pass);
  void InitializeQualifiedResourceNameTypeConverter(
      const std::map<int, std::string>&);
  QualifiedResourceNameTypeConverter* GetQualifiedResourceNameTypeConverter()
      const
  {
    return qualified_resource_name_type_converter_.get();
  }
  static bool GetTlsPskByFullyQualifiedResourceName(
      ConfigurationParser* config,
      const char* fully_qualified_name,
      std::string& psk);
  bool GetConfiguredTlsPolicyFromCleartextHello(
      const std::string& r_code,
      const std::string& name,
      TlsPolicy& tls_policy_out) const;

 private:
  ConfigurationParser(const ConfigurationParser&) = delete;
  ConfigurationParser operator=(const ConfigurationParser&) = delete;

 private:
  enum unit_type
  {
    STORE_SIZE,
    STORE_SPEED
  };

  std::string config_default_filename_; /* default config filename, that is
                                           used, if no filename is given */
  std::string config_dir_; /* base directory of configuration files */
  std::string
      config_include_dir_;      /* rel. path to the config include directory
                                    (bareos-dir.d, bareos-sd.d, bareos-fd.d, ...) */
  bool use_config_include_dir_; /* Use the config include directory */
  std::string config_include_naming_format_; /* Format string for file paths of
                                                resources */
  std::string used_config_path_;             /* Config file that is used. */
  std::unique_ptr<QualifiedResourceNameTypeConverter>
      qualified_resource_name_type_converter_;
  ParseConfigReadyCb_t ParseConfigReadyCb_;
  bool parser_first_run_;


  const char* get_default_configdir();
  bool GetConfigFile(PoolMem& full_path,
                     const char* config_dir,
                     const char* config_filename);
  bool GetConfigIncludePath(PoolMem& full_path, const char* config_dir);
  bool FindConfigPath(PoolMem& full_path);
  int GetResourceTableIndex(int resource_type);
  void StoreMsgs(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreName(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreStrname(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreStr(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreStdstr(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreDir(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreStdstrdir(LEX* lc, ResourceItem* item, int index, int pass);
  void store_md5password(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreClearpassword(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreRes(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreAlistRes(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreAlistStr(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreAlistDir(LEX* lc, ResourceItem* item, int index, int pass);
  void StorePluginNames(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreDefs(LEX* lc, ResourceItem* item, int index, int pass);
  void store_int16(LEX* lc, ResourceItem* item, int index, int pass);
  void store_int32(LEX* lc, ResourceItem* item, int index, int pass);
  void store_pint16(LEX* lc, ResourceItem* item, int index, int pass);
  void store_pint32(LEX* lc, ResourceItem* item, int index, int pass);
  void store_int64(LEX* lc, ResourceItem* item, int index, int pass);
  void store_int_unit(LEX* lc,
                      ResourceItem* item,
                      int index,
                      int pass,
                      bool size32,
                      enum unit_type type);
  void store_size32(LEX* lc, ResourceItem* item, int index, int pass);
  void store_size64(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreSpeed(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreTime(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreBit(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreBool(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreLabel(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreAddresses(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreAddressesAddress(LEX* lc, ResourceItem* item, int index, int pass);
  void StoreAddressesPort(LEX* lc, ResourceItem* item, int index, int pass);
  void scan_types(LEX* lc,
                  MessagesResource* msg,
                  int dest_code,
                  char* where,
                  char* cmd,
                  char* timestamp_format);
  TlsPolicy GetTlsPolicyForRootConsole() const;
  TlsPolicy GetTlsPolicyForJob(const std::string& name) const;
  TlsPolicy GetTlsPolicyForResourceCodeAndName(const std::string& r_code_str,
                                               const std::string& name) const;
};

void PrintMessage(void* sock, const char* fmt, ...);
bool IsTlsConfigured(TlsResource* tls_resource);

/*
 * Data type routines
 */
DatatypeName* get_datatype(int number);
const char* datatype_to_str(int type);
const char* datatype_to_description(int type);

/*
 * Resource routines
 */
void IndentConfigItem(PoolMem& cfg_str,
                      int level,
                      const char* config_item,
                      bool inherited = false);
void InitResource(int type, ResourceItem* item);

/*
 * Loop through each resource of type, returning in var
 */
#ifdef HAVE_TYPEOF
#define foreach_res(var, type)                                    \
  for ((var) = NULL; ((var) = (typeof(var))my_config->GetNextRes( \
                          (type), (CommonResourceHeader*)var));)
#else
#define foreach_res(var, type)                                        \
  for (var = NULL; (*((void**)&(var)) = (void*)my_config->GetNextRes( \
                        (type), (CommonResourceHeader*)var));)
#endif

#define LockRes(x) (x)->b_LockRes(__FILE__, __LINE__)
#define UnlockRes(x) (x)->b_UnlockRes(__FILE__, __LINE__)

#endif  // BAREOS_CORE_SRC_LIB_PARSE_CONF_H_
