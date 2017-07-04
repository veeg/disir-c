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
        struct semantic_version mo_version;
        //! Override value
        Json::Value mo_value;
    };
    //! semver comparator
    struct cmp_semver
    {
        bool operator()(struct semantic_version a, struct semantic_version b)
        {
            return dc_semantic_version_compare (&a, &b) < 0;
        }
    };
    // mo_version comparator
    struct cmp_mo_version
    {
        bool operator()(const std::unique_ptr<struct mold_entry_override>& a,
                        const std::unique_ptr<struct mold_entry_override>& b)
        {
            return dc_semantic_version_compare (&a.get()->mo_version, &b.get()->mo_version) < 0;
        }
    };
    //! rename map to hold synchronization override mapping
    using syncmap = std::map<struct semantic_version, struct semantic_version, cmp_semver>;

    class MoldOverride
    {
    public:
        //! Default contructor
        MoldOverride ();

        ~MoldOverride () {}

        enum disir_status
        apply_overrides (struct disir_context *context);

        enum disir_status
        parse_mold_override_entry (struct disir_instance *instance, std::istream& entry);

    private:
        enum disir_status
        string_to_semantic_version (Json::Value& current, const char *attribute_key,
                                    struct semantic_version *semver);
        enum disir_status
        validate_override_entry (struct disir_instance *intance, Json::Value& root);

        enum disir_status
        parse_mold_overrides (Json::Value& root);

        enum disir_status
        populate_mold_entry_override (Json::Value& current, std::string name,
                                      syncmap& override_mapping);

        enum disir_status
        validate_override (struct disir_instance *instance, 
                           Json::Value& current, std::string& name,
                           std::set<std::string>& versions);

        enum disir_status
        retrieve_highest_default_version (struct disir_context *context,
                                         struct semantic_version *highest_semver);

        enum disir_status
        parse_override_sync_mapping (Json::Value& sync_entries, syncmap& mapping);

        enum disir_status
        set_value_existing_default (struct disir_context *context_keyval,
                                    struct semantic_version *semver, Json::Value& value);

        enum disir_status
        apply_override (struct disir_context *context_mold, struct mold_entry_override *entry,
                        struct semantic_version *mold_version);

    private:
        std::map<std::string,
                 std::set<std::unique_ptr<struct mold_entry_override>, cmp_mo_version>> m_override_entries;
    };
}

#endif // DIO_JSON_MOLD_OVERRIDE_H
