# Webserv (CODAM : School42)

HTTP/1.1 web server implementation in C++;

## Team

- **I.Pavlov** - [12Ivan03](https://github.com/12Ivan03)
- **A.Erokhina** - [agerokhina](https://github.com/agerokhina)

## Workflow

- Feature branches
- Pull Requests
- Mandatory code review

## Build

```sh
make
```

## POST: первая версия загрузки

`POST` сохраняет всё тело HTTP-запроса как один бинарный файл в каталог из
`upload_store`. Обработчик не разбирает `multipart/form-data`,
`application/x-www-form-urlencoded` или MIME: заголовок `Content-Type` для этой
версии не влияет на сохранённые данные.

Для location должны быть разрешены `POST` и существовать доступный для записи
каталог:

```nginx
location /upload {
    allow_methods GET POST DELETE;
    upload_store ./site/www/uploads;
}
```

Сервер не создаёт `upload_store` автоматически. Например, перед запуском можно
подготовить каталог так:

```sh
mkdir -p site/www/uploads
```

### Пример с curl

`--data-binary` важен: он передаёт файл без преобразования переводов строк и
подходит также для не-текстовых данных.

```sh
curl -i --request POST \
  --data-binary @./photo.webp \
  http://127.0.0.1:8080/upload
```

После успеха сервер создаёт уникальный файл вида
`upload-<time>-<pid>-<attempt>` в `upload_store` и отвечает, например:

```http
HTTP/1.1 201 Created
Content-Length: 0
Location: /upload/upload-<time>-<pid>-<attempt>
Connection: keep-alive
```

### Пример сырого HTTP-запроса

Для тела `hello world` длина равна 11 байтам, поэтому `Content-Length` должен
быть `11`:

```http
POST /upload HTTP/1.1
Host: localhost:8080
Content-Length: 11

hello world
```

При разборе запроса `HTTPRequest` запоминает ссылку на уже накопленный request
buffer, смещение начала тела и его размер. `handlePost()` записывает этот
диапазон напрямую в файл; отдельный буфер для body не создаётся. Он вызывается
только после того, как подсистема приёма уже признала запрос полным.

## Tests

```sh
make test
```

Набор включает проверку границ body в исходном request buffer и POST-загрузку
бинарного тела с нулевым байтом: проверяется ответ `201 Created` и побайтовое
совпадение сохранённого файла с телом запроса.
