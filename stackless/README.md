# Stackless coroutines

Во всех задачах требуется использовать примитивы `lines`.

## Задача stackless/async_task

В данной задаче необходимо реализовать класс `AsyncTask`, который реализует `stackless` корутины с lazy семантикой. Laziness означает следующее: сначала у задачи планируется continuation, и только затем происходит запуск самой задачи. Это позволяет избавиться от необходимости синхронизации между двумя тасками.

В C++ корутинах довольно много точек кастомизации, но глобально их можно разделить на две группы: кастомизации через `promise_type` и кастомизация через `awaitable`.

### Promise

Отвечает за создание и завершение корутины. Promise выделяется вместе с состоянием корутины на куче в рамках одной аллокации, поэтому его время жизни связано с временем жизни самой корутины.

C++20 `[dcl.fct.def.coroutine]`:
> A coroutine behaves as if its function-body were replaced by
```cpp
{
    promise-type promise promise-constructor-arguments ;
    try {
        co_await promise.initial_suspend();
        function-body
    } catch ( ... ) {
        if (!initial-await-resume-called)
            throw;
        promise.unhandled_exception();
    }
    final-suspend:
        co_await promise.final_suspend();
}
```

### Awaitable

Отвечает за suspend корутины. Позволяет получить легковестный указатель на стейт уснувшей корутины `std::coroutine_handle`.

В теле корутины `co_await expr;` работает так, как если бы это выражение было на заменено на такой псевдо-код:
```cpp
{
  auto&& value = <expr>;
  auto&& awaiter = get_awaiter(get_awaitable(promise, value));
  if (!awaiter.await_ready())
  {
    <suspend-coroutine>

    awaiter.await_suspend(std::coroutine_handle<P>::from_promise(p));
    <return-to-caller-or-resumer>

    <resume-point>
  }
  return awaiter.await_resume();
}
```

### Ссылки

- [cppreference](https://en.cppreference.com/w/cpp/language/coroutines)
- [Symmetric Transfer](https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer)
