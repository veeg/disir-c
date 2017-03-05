#ifndef STATUS_CODES_H
#define STATUS_CODES_H


enum dplugin_status
{
    //! Success
    DPLUGIN_STATUS_OK = 0,

    //! If a json file contains error
    DPLUGIN_PARSE_ERROR,

    //! Non-recoverable error
    DPLUGIN_FATAL_ERROR,

    //! Error returned on error
    //! with disk interaction
    DPLUGIN_IO_ERROR,

    DPLUGIN_STATUS_FAILED,

    //
    DPLUGIN_STATUS_INVALID_CONTEXT,
};

#endif
