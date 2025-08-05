#include "reload_conf_handle.h"

namespace oeaware {
    
EventResult oeaware::ReloadConfHandler::Handle(const Event &event)
{
    std::string err;
    int levelBefore = config->GetLogLevel();
    if (!config->Reload("/etc/oeAware/config.yaml", err)) {
        WARN(logger, "reload config error: " + err);
        return EventResult(Opt::RESPONSE_ERROR, {err});
    }
    
    int logLevel = config->GetLogLevel();
    if (!Logger::GetInstance().ChangeAllLogLevel(logLevel)) {
        err = "reload log level error";
        WARN(logger, "reload log level error: " + err);
        return EventResult(Opt::RESPONSE_ERROR, {err});
    }
    std::string ret = "log level changed from "
        + std::to_string(levelBefore) + " to " + std::to_string(logLevel) + ".";
    // 使用Main日志打印，因为Main日志是INFO级别，不会被其他日志屏蔽
    auto loggerMain = Logger::GetInstance().Get("Main");
    INFO(loggerMain, ret);
    return EventResult(Opt::RESPONSE_OK, {ret});
}
}