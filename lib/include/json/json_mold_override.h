#ifndef DIO_JSON_MOLD_OVERRIDE_H
#define DIO_JSON_MOLD_OVERRIDE_H

// Public json
#include <json/json.h>

// Disir
#include <disir/disir.h>

// cpp standard
#include <memory>
#include <set>

namespace dio
{
    //! Defines an override entry
    struct mold_entry_override
    {
        //! keyval name absolute
        std::string mo_name;
        //! Namespace override version
        struct disir_version mo_version;
        //! Override value
        Json::Value mo_value;
    };
    //! version comparator
    struct cmp_version
    {
        bool operator()(struct disir_version a, struct disir_version b)
        {
            return dc_version_compare (&a, &b) < 0;
        }
    };
    // mo_version comparator
    struct cmp_mo_version
    {
        bool operator()(const std::unique_ptr<struct mold_entry_override>& a,
                        const std::unique_ptr<struct mold_entry_override>& b)
        {
            return dc_version_compare (&a.get()->mo_version, &b.get()->mo_version) < 0;
        }
    };
    //! rename map to hold synchronization override mapping
    using syncmap = std::map<struct disir_version, struct disir_version, cmp_version>;

    class MoldOverride
    {
    public:
        //! Default contructor
        MoldOverride ();

        ~MoldOverride () {}

        //! \brief Apply overrides parsed from a mold override entry.
        //!
        //! param[in] context Mold root context.
        //!
        //! \return DISIR_STATUS_OK on success.
        //! \return DISIR_INVALID_CONTEXT if applying overrides
        //!     results in violating invariants in mold.
        //!
        enum disir_status
        apply_overrides (struct disir_context *context);

        //! \brief Unserialize mold override entry.
        //!
        //! param[in] instance The disir instance.
        //! param[in] entry Stream containing the mold override entry.
        //!
        //! \return DISIR_STATUS_OK on success.
        //! \return DISIR_STATUS_NOT_EXIST if entry is not a mold override entry.
        //! \return DISIR_STATUS_FS_ERROR if either entry is invalid or unparseable.
        //!
        enum disir_status
        parse_mold_override_entry (struct disir_instance *instance, std::istream& entry);

    private:

        //! \brief Unserialize disir version
        enum disir_status
        string_to_disir_version (Json::Value& current, const char *attribute_key,
                                 struct disir_version *version);

        //! \brief Validate overrides in mold override entry
        enum disir_status
        validate_override_entry (struct disir_instance *intance, Json::Value& root);

        //! \brief Unserialize override keyvals
        enum disir_status
        parse_mold_overrides (Json::Value& root);

        //! \brief Populate map of overrides with keyval
        enum disir_status
        populate_mold_entry_override (Json::Value& current, std::string name,
                                      syncmap& override_mapping);

        //! \brief Validate a single override keyval entry
        enum disir_status
        validate_override (struct disir_instance *instance, Json::Value& current,
                           std::string& name, std::set<std::string>& versions);

        //! \brief Retrieve the newest default context from keyval context
        enum disir_status
        retrieve_highest_default_version (struct disir_context *context,
                                          struct disir_version *highest_version);

        //! \brief Unserialize and validate synchronization mapping
        enum disir_status
        parse_override_sync_mapping (Json::Value& sync_entries, syncmap& mapping);

        //! \brief Override value on keyval context
        enum disir_status
        set_value_existing_default (struct disir_context *context_keyval,
                                    struct disir_version *version, Json::Value& value);

        //! \brief Apply a single override keyval to mold
        enum disir_status
        apply_override (struct disir_context *context_mold, struct mold_entry_override *entry,
                        struct disir_version *mold_version);

    private:
        std::map<std::string, std::set<std::unique_ptr<struct mold_entry_override>,
                                                       cmp_mo_version>>  m_override_entries;
    };
}

#endif // DIO_JSON_MOLD_OVERRIDE_H
