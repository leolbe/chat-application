# chat-application

The goal of this project is to implement a complete end-to-end encrypted 
graphical chat application. This is the current plan:

1. Client-Server communication through CLI:
    - HTTP REST: Login, history fetching
    - SSE: Instant messaging
2. User accounts, persistent storage
    - SQLite
3. Client to client communication, conversations
4. End-to-end encryption
5. Graphical frontend (Qt or React web app, we'll see)

## Usage

Build and run using [Just](https://github.com/casey/just):

`just config`
`just run-server`
