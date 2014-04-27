#!/usr/bin/env python
"""A python script for a matchmaking game server for the Steamy project."""

from collections import deque
from select import select
from socket import *
from struct import pack, unpack, calcsize

def server_respond(server, gamerooms):
    """Accept on server and read handshake from accepted connection"""
    conn, address = server.accept()
    # Receive 32 bytes from server
    handshake_buffer = conn.recv(32)
    # unpack two unsigned integers and a null-padded string
    steamy_vers,game_vers,game_name = unpack("!IIs",handshake_buffer)
    game_name = game_name.partition(b'\0')[0] # unpad NULLs from string

    # For a valid game name, add player to deque and return
    if game_name in gamerooms:
        conn.send(":D")
        gamerooms[game_name].append((conn, address))
        return (conn, address)

    else: # if name of game is invalid, reject
        conn.send(":(")
        conn.close()
        return None


def check_gamerooms(gamerooms):
    """Given a gameroom dict of deques, if a deque is full, start the given game
    and empty then empty the deque."""
    for game, room in gamerooms:
        if count(room) == room.maxlen:
            start_game(game, room)

def start_game(game, room):
    """Given a full deque for a game room, start the server of the given game
    name and pipe into it the address info of the clients. The game server will
    respond in kind with its socket address information, which should be used to
    contact the individual game clients."""
    pass

def main():
    # create a nonblocking server on 
    server = socket(AF_INET, SOCK_STREAM)
    server.setblocking(0)
    server.bind(('localhost',5774))
    server.listen(1)

    # create a list of sockets to listen on
    inputs = [ server ]
    outputs = []

    # create a dictionary of game room deques
    gamerooms = { "snake" : deque(maxlen=2) } 

    # loop forever
    while inputs:
        readable, writable, exceptional = select(inputs, outputs, inputs, 0.1)

        # if server has an incoming connection, accept on it
        if server in readable:
            client = server_respond(server, gamerooms)
            readable.remove(server)

        # handle connections from any existing hosts
        for client in readable:
            pass

        # check whether any games are ready to start
        check_gamerooms(gamerooms)

if __name__=="__main__":
    main()