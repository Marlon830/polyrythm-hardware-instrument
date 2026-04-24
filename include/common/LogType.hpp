#ifndef COMMON_LOGTYPE_HPP
    #define COMMON_LOGTYPE_HPP

    namespace Common {
        /// @brief Enumeration of log message types.
        /// in a separate file to avoid circular dependencies.
        enum logType { INFO, WARNING, ERROR };
    }

#endif // COMMON_LOGTYPE_HPP