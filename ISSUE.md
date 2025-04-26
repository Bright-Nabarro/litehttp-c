# 缺陷
- [x] `camke` 需要添加`khash`依赖
- [ ] `c_thread_pool`使用了`FetchContent`, 编译耗时，使用`git`子模块替代
- [ ] `malloc` / `realloc` 错误时会将消息传递到logger, 导致更多的malloc分配，
需要修改logger，增加紧急日志方法
- [ ] `http_types` 和 `http_requests`中缺少错误处理和错误输出

# 功能需求
- [ ] `keepalive` 未实现
- [ ] `http request` 回应
- [ ] `http head method` 实现
- [ ] 添加`int_to_string_t`函数
- [ ] (耗时) 添加`tcp`粘包，拆包处理
- [ ] 扩展`http`方法
- [ ] 扩展`http`首部
- [ ] `toml`对于ip配置扩展为列表
- [ ] `rio_write` 和 `rio_read`
- [ ] `uitl/base.h` 添加各种错误处理宏，简化代码
- [ ] 添加服务器监听所有`ip`配置
- [ ] `string_view_t` 和 `string_t` 取消对于空指针传入的支持
- [ ] 手动实现线程安全，带细粒度锁的哈希库 -> 到`c_thread_pool`中
- [ ] 读取`html`使用mmap,
- [ ] (耗时) 读取`html`使用lru算法
- [ ] (耗时) 添加超时关闭连接，使用最小堆，事件驱动

# 重构
- [x] `http_err_t`对于一些方法失败的名称改成`http_err`前缀
- [x] `configure.h`中方法添加`config_`前缀
- [ ] `http`中部分方法命名随意，需要规范前缀为`http`
- [ ] `http_parser` 将上下文类暴露到接口，配合`tcp`粘包，拆包实现, 其接口直接重构
- [ ] `client_recv` 相关的读取函数重写，配合`tcp`粘包，拆包实现
- [ ] 明确`http_parser`解析状态机，划分责任

# 待测试
- [ ] `string_view_t` 新增功能

