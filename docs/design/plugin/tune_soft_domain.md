
## 插件依赖项

依赖系统docker信息


## 代码变更记录

1、生成插件框架(by AI)
```
提示词：
我需要增加一个soft_domain_tune调优插件，请参考 plugin/system 目录下的插件，实现相应的接口，不需要完成具体功能，要求生成插件后可以构建出插件。请登录远程服务器执行build.sh确保编译成功。
```

2、订阅docker数据(by AI)

```
提示词：
soft_domain_tune 插件依赖 系统docker 信息，该信息需要 docker_collect 插件获取，请订阅该采集项, 并将获取到的docker 信息存到soft_domain_tune 插件结构体，注意每个周期都要更新插件
```

3、订阅线程信息数据(by AI)

```
提示词：
soft_domain_tune 插件依赖 系统线程信息，该信息需要 thread_collect 插件获取，请订阅该采集项, 并将获取到的线程信息存到soft_domain_tune 插件结构体，注意每个周期都要更新插件
```