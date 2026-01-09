
## 插件依赖项

+ 依赖系统docker信息
+ 依赖系统线程信息
+ 依赖系统环境信息


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

4、解析并校验配置文件(by AI)
+ 采用yaml配置，配置文件在 `/etc/oeAware/plugin/soft_domain.yaml`
+ 默认什么也不配置
+ 配置有如下两种方式
```yaml
- type: "docker"
  whitelist: ["mysql*"]
  cpu_num: "16"
- type: "process"
  whitelist: ["mysql*"]
  cpu_num: "16"
```
```
提示词：按照上述配置文件格式，将配置的元素解析出来，将type定义为枚举，whitelist 定义为 字符串，用vector 存储配置，每个元素是一个结构体。

全部解析后进行校验：cpu num 不能超过单个numa的cpu个数
numa、cpu 信息需要订阅 @env_data.h ，请不要自行实现
增加soft_domain.yaml 配置文件

由于enable时采集数据可能尚未获取，但是numa内cpu个数比较容易获取，可以在enable 时通过 config_cpu_num / numa_num 换算 单numa的cpu个数，在update时更新一次static信息即可，因为信息后面不会变
```
