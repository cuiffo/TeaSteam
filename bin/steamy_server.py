#!/usr/bin/env python
"""A python script for a matchmaking game server for the Steamy project."""

#from collections import deque
from os import path
from select import select
from socket import *
from struct import pack, unpack, calcsize
from subprocess import Popen, PIPE

# thank you stackoverflow and muhammad alkarouri
import collections

class deque(collections.deque):
    def __init__(self, iterable=(), maxlen=None):
        super(deque, self).__init__(iterable, maxlen)
        self._maxlen = maxlen
    @property
    def maxlen(self):
       return self._maxlen

def server_respond(server, gamerooms):
    """Accept on server and read handshake from accepted connection"""
    conn, address = server.accept()
    # Receive 32 bytes from server
    handshake_buffer = conn.recv(32)
    # unpack two unsigned integers and a null-padded string
    steamy_vers,game_vers,game_name = unpack("!II24s",handshake_buffer)
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
    """Given a gameroom dict of deques, if a deque is full, start the given
    game, empty the deque, and return the process object."""

    return [start_game(g,r) for g, r in gamerooms.items() if len(r)==r.maxlen]

def start_game(game, room):
    """Given a full deque for a game room, start the server of the given game
    name and pipe into it the address info of the clients. The game server will
    respond in kind with its socket address information, which should be used to
    contact the individual game clients."""

    gamepath = path.join(path.dirname(__file__), game+"_server")
    p = Popen([gamepath],stdin=PIPE,stdout=PIPE)

    # Read the server port as a uint16 and transmit to clients
    port = unpack("=H",p.stdout.read(2))[0]
    print("The port number appears to be {0}".format(port))

    # Write the addresses of the clients to the new process
    # client[0] is a socket
    # client[1] is an address tuple: (hostname, port)
    for client in room:
        p.stdin.write(
            inet_aton(client[1][0]) # convert quad-dot formatted address to binary
            + pack("!H",client[1][1])) # pack port into a uint16 in network byte order
        client[0].send(pack("!H",port))
        client[0].close()

    # close input stream on process since we're done
    p.stdin.close()

    # clear deque
    room.clear()

    return p

def wait_finished_servers(servers_list):
    """Given a list of process objects for game servers, poll the servers for
    their completion status and wait() and remove() finished ones."""
    # for server in servers_list:
    #     if server.poll() is not None:
    #         server.wait()
    #         servers_list.remove(server)
    [s.wait() for s in servers_list if s.poll() is not None]
    servers_list[:] = [s for s in servers_list if s.poll() is None]

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
    gamerooms = { "connectFour" : deque(maxlen=2) }

    # create a list of active game servers
    activeservers = []

    # loop forever
    while inputs:
        readable, writable, exceptional = select(inputs, outputs, inputs, 0.1)

        # if server has an incoming connection, accept on it and add to room
        if server in readable:
            client = server_respond(server, gamerooms)
            readable.remove(server)

        # handle connections from any existing hosts
        for client in readable:
            pass

        # check whether any games are ready to start
        activeservers.extend(check_gamerooms(gamerooms))

        # wait on all terminated processes only if they are indeed finished
        wait_finished_servers(activeservers)

if __name__=="__main__":
    main()
