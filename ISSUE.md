# 缺陷
- [x] `cmake` 需要添加`khash`依赖
- [ ] `http_response` 使用`cc_vec`包装
- [ ] `c_thread_pool`使用了`FetchContent`, 编译耗时，使用`git`子模块替代
- [ ] `malloc` / `realloc` 错误时会将消息传递到logger, 导致更多的malloc分配，
需要修改logger，增加紧急日志方法
- [ ] `http_types` 和 `http_requests`中缺少错误处理和错误输出
- [ ] `accept` 处理有问题

# 功能需求
- [x] `http request` 回应
- [x] 添加`int_to_string_t`函数
- [x] (耗时) 添加`tcp`粘包，拆包处理
- [x] `keepalive` 实现
- [x] `http head method` 实现
- [ ] `http request`对于分块传输的解析
- [ ] `toml`对于ip配置扩展为列表
- [ ] `rio_write` 和 `rio_read`
- [ ] `uitl/base.h` 添加各种错误处理宏，简化代码
- [ ] `string_view_t` 和 `string_t` 取消对于空指针传入的支持
- [ ] 手动实现线程安全，带细粒度锁的哈希库 -> 到`c_thread_pool`中
- [ ] 读取`html`使用mmap,
- [ ] (耗时) 读取`html`使用`lru`算法
- [ ] (耗时) 添加超时关闭连接，使用最小堆，事件驱动
- [ ] 添加`EPOLL_OUT`对客户端写入
- [ ] 具体请求头对应方法解析不完全

# 重构
- [x] `http_err_t`对于一些方法失败的名称改成`http_err`前缀
- [x] `configure.h`中方法添加`config_`前缀
- [x] `http`中部分方法命名随意，需要规范前缀为`http`
- [ ] `http types`中的方法写成`def`形式
- [ ] `http_request` 和 `http_response` 的 `header_t` 用`cc_vec`封装
- [ ] 明确`http_parser`解析状态机，划分责任
- [ ] `http_parser` 将上下文类暴露到接口，配合`tcp`粘包，拆包实现, 其接口直接重构
- [ ] `client_recv` 相关的读取函数重写，配合`tcp`粘包，拆包实现
- [ ] 在堆上创建对象使用`new`~`free`，其他静态变量使用`init`~`deinit`，统一命名，取消`create`, `destroy`, `release`命名

# 待测试
- [x] `string_view_t` 新增功能
- [ ] `core`
- [ ] `http` tcp粘包解析测试
- [ ] `http` tcp拆包解析测试
