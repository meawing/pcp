# Golang buffered channel

Реализуйте аналог каналов из языка `Go`

Для такого канала задается размер буфера. `Push` блокируется, пока буфер канала заполнен. `Pop` блокируется, если канал пуст.

`Close` позволяет закрыть канал. Последующие вызовы `Push` не имеют эффекта, `Pop` возвращает пустой `optional`.

Вы можете сами дать название такому каналу в терминах из задачи `mpmc-queue`

## Ссылки

[go channel](https://go.dev/tour/concurrency/3)
[optional](https://en.cppreference.com/w/cpp/utility/optional)
