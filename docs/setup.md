# Настройка окружения для сдачи и решения задач

## Регистрация в manytask

Прежде всего необходимо зарегистрироваться на курсе. Для этого необходимо перейти по адресу https://pcp.manytask.org и зарегистрироваться, либо авторизоваться (если вы уже работали с manytask раньше). Кодовое слово для регистрации указано на странице [lms](https://lk.yandexdataschool.ru/courses/2025-spring/7.1347-parallel-concurrent/).

## Настройка репозитория

### Добавление ssh-ключа

После регистрации для вас будет создан личный репозиторий на gitlab.manytask.org, который необходимо себе склонировать. Но перед этим необходимо сгенерировать ssh-ключ и добавить его в аккаунт на gitlab.manytask.org (если у вас его там нет). Можно воспользоваться [туториалом от gitlab](https://docs.gitlab.com/ee/user/ssh.html), либо проследовать по инструкции ниже.

1. Сгенерируйте ключ (возможно придется поставить `openssh-client`): `ssh-keygen -t ed25519 -f ~/.ssh/manytask_ed25519`
2. Выведите содержимое **публичного** ключа и скопируйте его: `cat ~/.ssh/manytask_ed25519.pub`
3. Перейдите на gitlab.manytask.org -> нажмите на иконку с вашим профилем в правом верхнем углу -> `Preferences` -> слева нажмите на `SSH keys`
4. Добавьте скопированный ключ в форму и нажмите `Add key`
5. Выполните следующую команду, чтобы использовался ваш новый ключ при подключении к gitlab.manytask.org:
`echo 'Host gitlab.manytask.org\n\tIdentityFile ~/.ssh/manytask_ed25519' >> ~/.ssh/config`

### Настройка репозитория

```sh
# Склонируйте свой репозиторий в нужную директорию
git clone git@gitlab.manytask.org:pcp/students-2025-spring/<логин на manytask>.git

# Перейдите в директорию склонированного репозитория
cd <репозиторий>

# Настройте возможность получать обновления из публичного репозитория с задачами
git remote add upstream git@gitlab.manytask.org:pcp/public-2025-spring.git
```

## Настройка окружения

Для настройки воспроизводимого окружения мы предлагаем использовать пакетный менеджер [nix](https://nixos.org/explore).

Чтобы настроить окружение, запустите `source ./env/activate` из корня репозитория. Скрипт `activate` установит пакетный менеджер `nix`, скачает необходимые для курса пакеты (это не повлияет на уже установленные в систему пакеты) и запустит новый `shell` с правильным образом настроенными переменными окружения. После настройки окружения в терминале появятся клиент курса `cli`.

Клиент курса `cli` - это легковесная обертка над `cmake`, `clang-format` и `clang-tidy` для удобного запуска тестов и линтеров. Полный список команд можно посмотреть вызовом команды `cli --help`. При желании вы можете компилировать и запускать тесты вручную, однако `cli` используется для запуска тестов в системе автотестирования, поэтому для идентичных результатов мы рекомендуем пользоваться клиентом.

## Тестирование задач

Для просмотра полного списка доступных задач рекомендуется использовать команду `cli list-tasks`. Для запуска тестов одной из этих задач необходимо сначала перейти в соответствующую директорию с задачей и затем выполнить команду `cli test` в терминале.

Все задачи в курсе описаны в файлах с расширением `.task.yml`. Каждая задача может включать одну или несколько целей `cmake`. Кроме того, каждая задача определяет набор файлов, допускаемых для отправки (поле `submit_files`). *В систему автотестирования будут отправлены только те файлы, которые перечислены в `submit_files`. Это означает, что изменения, внесенные в другие файлы, не будут учтены в процессе автоматического тестирования.*

Команда `cli test` предоставляет возможность выбора профилей, под которыми следует запускать тесты, а также устанавливать фильтры по именам тестов. Например, следующая команда запустит тесты с профилями `release` и `release-lines`, включая только тесты `SetVoid` и те, которые соответствуют паттерну `SetE*`: `cli test -p release -p release-lines -f SetVoid -f 'SetE*'`.

## Обновление репозитория

Для получения новых заданий и тестов нужно выполнить `git pull upstream main`. Это загрузит последние обновления из публичного репозитория. После обновления репозитория желательно перезапустить окружение (`source env/activate`).

Если есть staging изменения, то выполнить следующие команды и разрешить merge-conflict-ы (если они есть):
```sh
git stash
git pull upstream main
git stash pop
```

## Отправка решений

Для того чтобы отправить одну или несколько задач на проверку, необходимо закоммитить ее в ваш репозиторий и отправить изменения в gitlab:
```sh
git add spinlock/threads/spinlock.h
git commit -m 'Add spinlock task'
git push origin main
```

За процессом тестирования можно следить на странице `CI/CD -> Jobs` в своём репозитории. Там же будут показаны статусы посылок и результаты тестирования.

Проверка задач происходит следующим образом:

1. Система смотрит на изменения в последнем коммите и находит задачи, которые затронуты этими изменениями (с помощью поля `submit_files`). *Если в последнем коммите код задачи не был затронут, то эта задача не будет протестирована.*
2. Файлы измененных задач копируются из репозитория студента в приватный репозиторий и затем запускаются тесты.

Если хоть одна задача падает на тестах, в интерфейсе гитлаба запуск будет считаться неудавшимся (failed). Если какая-либо задача в комплекте прошла тесты, баллы за неё поставятся в систему независимо от остальных.

## Настройка автокомплита в VSCode

1. Установите плагин [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd). При установке плагина скачивать `clangd` сервер необязательно.
2. Выполните команду `source ./env/activate --command which clangd` из корня репозитория. Скопируйте выведенный путь.
3. Добавьте атрибут `"clangd.path"` с полученным на предыдущем шаге путем в конфиг `<pcp-repo>/.vscode/settings.json`. Также добавьте атрибут `"editor.defaultFormatter"` для корректной работы команды `Format Document` из `Command Pallete`. Должно получиться примерно так:
```json
{
    "clangd.path": "/nix/store/cpdnl8zlvdka0qpfifs5h2088q6vlf7z-clang-16.0.6/bin/clangd",
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd"
}
```
4. Активируйте окружение и сконфигурирyйте все задания с курса: `cli configure`.
5. В корне репозитория появится несколько директорий вида `build-<profile>`. Каждая такая директория соответствуют одному из профилей сборки, которые указываются в `.task.yml`. По умолчанию используйте директорию `build-release-lines`. В ней находится файл `compile_commands.json`, скопируйте его в корень репозитория. Различным профилям сборки может соответствовать различный набор макросов компиляции, поэтому если вы столкнулись с тем, что автокомплит плохо работает, проверьте, что вы скопировали `compile_commands.json` из правильной директории.

## Настройка проекта в CLion

Прежде всего откройте проект с курсом в CLion. Должна появиться директория `.idea` в корне репозитория.

### Настройка автокомплита

1. Активируйте окружение и сконфигурируйте все задания с курса: `cli configure`.
2. В корне репозитория появится несколько директорий вида `build-<profile>`. Каждая такая директория соответствуют одному из профилей сборки, которые указываются в `.task.yml`. По умолчанию используйте директорию `build-release-lines`. В ней находится файл `compile_commands.json`, скопируйте его в корень репозитория. Различным профилям сборки может соответствовать различный набор макросов компиляции, поэтому если вы столкнулись с тем, что автокомплит плохо работает, проверьте, что вы скопировали `compile_commands.json` из правильной директории.

### Настройка целей сборки

Активируйте окружение и выполните команду, которая настроит цели сборки для CLion: `cli setup-clion`. После этого в CLion должен появиться список целей для запуска, а также заработать встроенный в IDE дебагер.

## Проблемы со сборкой/тестированием

В случае возникновения непонятных ошибок, как, например, такой `FATAL: ThreadSanitizer: unexpected memory mapping...`, попробуйте активировать окружение следующей командой: `source env/activate -i`. Опция `-i` включит игнорирование текущих переменных окружения, что в некоторых случаях может помочь.

## Обратная связь

Если в процессе прохождения курса вы столкнулись с трудностями или у вас есть предложения по улучшению платформы для сдачи и решения задач, пожалуйста, не стесняйтесь обращаться в чат курса. Мы открыты к вашим идеям и постараемся реализовать предложенные улучшения.
