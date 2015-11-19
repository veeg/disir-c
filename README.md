# Disir - Text-based configuration files management

_Disir_ is a text-based configuration file management system. Why? Because configuration files are made to be configured. _They change_. When the provider of a configuration file changes the semantics of a configurable variable, this change is not propagated to deployed configuration files through an upgrade. Heck, even the changes made by the user is not saved through an update.

### But we have RPM, which readily stores modified configuration files as *.rpmnew. Why not just use that?

Because its **manual**. It does not aid you in resolving conflict between the saved backup and the new version. It provides no documentation **alongside** the configuration definition. It does not automatically provide type safety and restriction checks. This is all left up to be manually handled by the application deploying a configuration file. 

### So what does _Disir_ provide?
* Semantically versioned, text-based configuration files - in any format. JSON, JSON-Lines, XML, INI.
* Update deployed configs with new variables, new defaults, new restrictions, whilst still keeping the changes made by the end user. If the changes are conflicting - we can provide the best tools to resolve them.
* Manage and monitor a multitude of configuration files deployed on your system. Each configuration file belongs to a group, which enables the user to easily manage loosely related configs belonging to the same group.

## How?

_Disir_ seperates the text-based configuration file and its on-disk representation away from its actual definition. Instead, the _Disir Config_ relies on a associated _Disir Schema_. No _Disir Config_ is valid without a schema. It defines all available key-value pairs, each sections, their documentation, default values and restrictions.
Since the configuration is defined solely by its schema, when a software package is installed that provides a configuration file, it only installs the schema, **not** the actual text-based configuration. That means, that if an existing version of the configuration file exists, the installation/update will not blindly overwrite

### _Disir Config_

Three key attributes within the _Disir Config_ associates it with _Disir_. 
* **Disir_version**: The configuration file can identify the deployed version of itself. If this attribute is not present, _Disir_ wiill assume the default semantic version number 1.0.0.
* **Disir_group**: The configuration file is associated with this named group. If no attribute is provided or it is empty, the **default** group is assigned.
* **Disir_schema_identifier**: The configuration file uses this explicitly named schema. The value here may be a normal identifier, or a complete filepath.

Note that any of these three attributes may be omitted - and _Disir_ will resolve to the default group, looking for a schema with the same identifier as the config.

#### Contents of _Disir Config_

* **Disir_config_attributes**, which only includes the named key/value pairs listed above.

* A **keyval** defines a named key, and associates it with a single value. The **keyval** name is not necessarily unique - whether or not this the case is defined by the _Disir Schema_.
* A **section** which contains one or more **keyval**s. The **section** is named by a canonical identifier, which cannot be the same as any **keyval**. There may be multiple **section**s with the same name active at once. This is again defined by the _Disir Schema_.

Both **keyval** and **section** may have a **_disabled_** attribute - This may not necessarily feasible to represent in all text-based formats, but still a feature of _Disir Config_. When the element is disabled, it will still be parsed and stored in the in-memory object representation, but it will not be validated or affect the state of the configuration file at all.

### _Disir Schema_

Defines all available **keyval**s and **section**s for a configuration file. Many of the elements within the schema are _versioned_, with an **introduced** and **deprecrated** attribute. This means that the whole history of the configuration file is expressed through the same schema file. One are able to update any old version of a configuration file, since all changes are recorded.

#### Key elements
* **Introduced**: At what semantic version of the schema this element  is valid from.
* **Deprecrated**: At what semantic version of the schema this element is no longer valid. If the semver of deprecated is less or equal than **introduced**, the the schema is **invalid**.
  * **Deprecation notice**: Why was this element deprecated? String explanation for it goes right here!
* **Documentation**: Text string documenting the intent behind this keyval
  * **introduced**: When was this documentation introduced! There may be multiple documentation entries to keyval. This is simply to update the documentation between major updates of the element. Documentation is not deprecated, only surpassed.

* **Value type**: What kind of value does this element hold. Only applicable to the *keyval* element.
  * Boolean
  * Integer
  * Float
  * String
* **Restriction**: What restrictions are in place either on the key or the value. The key can either conflict with existing keys, be required if another key exists. The value can also be restricted to hold a specific value of its type, or a range within it.


A **section** contains 
* **Introduced**
* **Deprecate**
* **Restriction**s
* **keyval**s

A **keyval** contains
* **Introduced**
* **Deprecated**
* **Documentation**
* **Value type**
* **Restriction**s

## Buzzwords / Dictionary

* **Identifier**: Both _Disir Config_ and _Disir Schema_ is referenced by an identifier. When opening a text-based configuration file, the identifier to open a _Disir Config_ will be the full filepath to its location on disk. Therefore, if non of the _Disir Config_ attributes are set within it, the associated _Disir Schema_ must reside in the default group under the same identifier as the _Disir Config_ filepath.


## Disir Group

The _Disir Group_ is a way to organize loosely related schemas and configuration files together. Each _Disir Schema_ must be categorized under a group. A _Disir Config_ is associated with a schema, which belongs to a group. The combination **(group, schema)** is therefore unique.

There are two reserved groups
* **default**: Schemas may be installed under the default group, of which all non-explicitly group configurations also reside.
* **all**: This group name has special semantics for some CLI operations, and may therefore not be used as an actual group name.

## Disir IO Interface

Since _Disir_ has an in-memory object model agnostic to the textual representation on disk, it provides a highly extensible I/O interface. It is a series of reader/writer operations that expect the I/O interface to perform a dedicated task to/from the disk/resource format it represents. As such, we provide interfaces for
* **XML** - read XML formated _Disir_ files and construct the in-memory object model. Write the in-memory object model to disk formatted as XML.
* **JSON** - read and write JSON formatted _Disir_ files to/from the in-memory object model.
* **INI** - read and write INI formatted _Disir files to/from the in-memory object model.

The operations that the I/O Interface must support are
```
// Read the configuration identified by the passed id and populate a configuration object.
dio_config_read(id)

// Write the config object to the location identified by the passed id.
dio_config_write(id, config)

// Return only the semantic version of this config file.
dio_config_version(id)

// Return only the group this config file is associated with.
dio_config_group(id)

// Read the schema located at the passed identifier, return the schema object.
dio_schema_read(id)

// Write the schema object identified by the passed id, in the appropriate group.
dio_schema_write(id, group, schema)
```

## Disir Context

TODO: How does the Disir context work - Its the main interface down to _Disir_
