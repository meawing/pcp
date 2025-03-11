# Flat Combining

В этом задании нужно реализовать примитив синхронизации [Flat Combining](https://people.csail.mit.edu/shanir/publications/Flat%20Combining%20SPAA%2010.pdf).

API комбайнера предполагается следующее:
```cpp
template <typename Variant, typename Dispatcher>
class FlatCombinerBase {       
public:
    FlatCombinerBase(int concurrency);
  
    struct Message {
        std::atomic<bool> request_sent = false;
        Variant body;
        char padding[64];
    };

    void* CheckIn();

protected:
    void Run(Message* message);

    ...
};

```

Класс комбайнера параметризуется двумя аргументами:
1. `Variant` -- тип данных, лежащий внутри сообщения, которое используется в `publication list`;
2. `Dispatcher` -- класс-наследник (для [CRTP](https://en.cppreference.com/w/cpp/language/crtp)).

В конструкторе класса необходимо задать количество потоков-пользователей -- это упрощение, чтобы было легко работать с `publication list`.

Для того, чтобы воспользоваться инстансом класса, каждый тред должен приложить к запросу `Cookie`.
`Cookie` можно получить, вызвав метод `CheckIn`.

Изучите примеры использования комбайнера из тестов.

Обратите внимание: `lines` использовать не обязательно.
