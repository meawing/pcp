# Coroutine

Реализуйте функциональность сопрограммы, используя `lines`. `Coroutine` или сопрограмма - это функция, которую можно останавливать и возобновлять.

Для этого есть метод `Coroutine-ы` `Resume` и статический метод `Coroutine::Suspend`

# Stackful

Для сохранения состояния исполнения в точке остановки нам понадобится `lines::Context` и, в частности, `lines::Stack`

# Stackless

С помощью переключения контекста несложно выразить механику сопрограммы. Cейчас строчки с вызовом `Coroutine::Suspend` превращают обычную функцию в корутину. Реализовав поддержку такой разметки на уровне компилятора можно избавиться от аллокаций стека. В С++20 был добавлен такой механизм - [stackless coroutines](https://en.cppreference.com/w/cpp/language/coroutines)

## Ссылки

[coroutines](https://www.baeldung.com/cs/coroutines-cooperative-programming)
