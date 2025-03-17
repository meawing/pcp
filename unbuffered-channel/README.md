# Golang unbuffered channel

Реализуйте аналог каналов из языка `Go`

`Push` и `Pop` блокируются пока противоположная сторона не готова к передаче элемента.

`Close` позволяет закрыть канал. Последующие вызовы `Push` бросают исключение, `Pop` возвращает пустой `optional`.

## Ссылки

[go channel](https://go.dev/tour/concurrency/2)
[optional](https://en.cppreference.com/w/cpp/utility/optional)
