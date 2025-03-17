# Multi-Producer Multi-Consumer Blocking Unbounded Queue

Предлагается реализовать `MPMCBlockingUnboundedQueue` с помощью `lines`. Семантика очереди такова:
- Методы `Push` и `Pop` могут быть вызваны из разных потоков (про это multi-producer и multi-consumer aka потоки вызывающие `Push` и `Pop` соответственно)
- `Push` всегда завершается успехом, даже если нет потоков, забирающих элементы из очереди (про это unbounded)
- `Pop` блокируется в случае, если очередь пуста (про это blocking)
