# HTTP Server in C

A lightweight HTTP/1.1 web server written in C using POSIX sockets. It serves static HTML files from a `/public` directory and handles basic HTTP response codes.

---

##  Screenshots

### Homepage (`GET /`)
![Homepage](./screenshots/homepage.png)

### 404 Page (`GET /missing.html`)
![404 Page](./screenshots/notfound.png)

---

## Features

- Parses basic `GET` HTTP/1.1 requests
- Serves static `.html` files from a `./public/` directory
- Returns proper HTTP status codes:
    - `200 OK` for found files
    - `404 Not Found` with a custom HTML page
    - `405 Method Not Allowed` for unsupported methods
    - `500 Internal Server Error` on file read failures
- Graceful error handling and clean connection closure

---

## Technologies Used

- **C**
- **POSIX sockets**
- **HTTP/1.1 protocol**
- **CMake** for build system

---

## How to Build & Run

From your project root, run the following command:

```bash
./run.sh
```
---

##  How to Test

### In your browser:

- Visit `http://localhost:8080/` - Should serve `public/index.html`
- Visit `http://localhost:8080/doesnotexist.html` - Should serve `public/404.html` (you can have any text after the /)

### Using `curl`:

```bash
curl -i http://localhost:8080/               /* 200 OK */
curl -i http://localhost:8080/missing.html   /* 404 Not Found */
curl -i -X POST http://localhost:8080/       /* 405 Method Not Allowed */
```

---

## To Add

- Support for `MIME` type
- Support for the HTTP `HEAD` method
- Potentially, multithreading
