# Future compositions

`future` позволяют не только синхронно дожидаться завершения операции, но и выстраивать параллельные и последовательные композиции асинхронных операций.

Tребуется использовать примитивы `lines`.

## Задача future/base

Реализуйте базовый функционал `Future`. Важным является наличие метода `Subscribe`, который позволяет "подписываться" на результат выполнения асинхронной операции:
```cpp
FooFuture(input).Subscribe([](Result<int>&& result) noexcept {
    std::cerr << "FooFuture has completed with value: " << *result << std::endl;
});
```
Данный метод скорее предназначен для реализации других методов композиции, чем для непосредственного использования, однако сделаем его публичным для простоты.

## Ссылки

- [Facebook's Futures](https://engineering.fb.com/2015/06/19/developer-tools/futures-for-c-11-at-facebook/)
- [Twitter's Futures](https://twitter.github.io/finagle/guide/Futures.html)
- [std::future](https://en.cppreference.com/w/cpp/thread/future)
