# KfxApiTool

A client to interact with the KeeperFX API.
It is mostly useful during mapmaking.
You can subscribe to variables and monitor them in real time.
You can also add preset commands to interact with the game.

It is crossplatform and made using Qt6 C++.

## Screenshot

![image](https://github.com/yani/KfxApiTool/assets/6956790/7428690d-0779-49ad-9316-c133440233ae)

## Features

- Set variables
- Subscribe to variables (live read)
- Subscribe to events (pretty useless atm)
- Execute map commands
- Execute console commands
- Auto reconnect (just leave it on and you can keep using the same vars/commands while working on a map)

## Usage

1. Set `API_ENABLED` to `TRUE` in your `keeperfx.cfg`
2. Take note of `API_PORT`, it's best to leave it at 5599 but you can change it if you want
3. Start a KeeperFX map
4. Connect to the API with KfxApiTool. You can use 'Connect to 127.0.0.1:5599' if you did not change the port.
5. Now you can monitor variables, change them and run commands

## Todo

- Read map data
- Save var/command presets (Ex: have a preset for each map)

## Author's note

This is my first test trying out the Qt framework. That's why it's quite messy. If this tool takes off I'll probably refactor it.
