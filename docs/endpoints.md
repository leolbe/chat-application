# Server endpoints

Version 1
- No confidentiality
- No session persistence

## Create user account

```http
POST /signup HTTP/1.1
Authorization: Basic {base64("$username:$password")}
```

### Responses

User created:
```http
HTTP/1.1 201 Created
```

Username already taken:
```http
HTTP/1.1 409 Conflict 
```

Invalid username or password:
```http
HTTP/1.1 422 Unprocessable Content
```

## Fetch message history

```http
GET /messages HTTP/1.1
Authorization: Basic {base64("$username:$password")}
```

### Responses

Message history fetched
```http
HTTP/1.1 200 OK
Content-Type: text/plain

{$messages[0]}
{$messages[1]}
{...}
```

Authorization failed:
```http
HTTP/1.1 401 Unauthorized
```

## Change username/password

```http
POST /change-login HTTP/1.1
Authorization: Basic {base64("$username:$password")}

{base64("$new_username:$new_password")}
``````

### Responses

Username/password changed:
```http
HTTP/1.1 204 No Content
```

Authorization failed:
```http
HTTP/1.1 401 Unauthorized
```

Username already taken:
```http
HTTP/1.1 409 Conflict 
```

Invalid username or password:
```http
HTTP/1.1 422 Unprocessable Content
```

## Instant messaging

```http
GET /chat HTTP/1.1
Authorization: Basic {base64("$username:$password")}
Connection: Upgrade
Upgrade: websocket
Sec-WebSocket-Version: 13
```

### Responses

Successful, switching to WebSocket:
```http
HTTP/1.1 101 Switching Protocols
Connection: Upgrade
Upgrade: websocket
```

Authorization failed:
```http
HTTP/1.1 401 Unauthorized
```
