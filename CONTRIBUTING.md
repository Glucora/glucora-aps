# Contribution Guide

## Table of content

- [Requiriements](#requirements)
- [Project Structure](#project-structure)
    - [Folder structure](#currentfolder-structure)
    - [Notes about specific folders](#note)
    - [Components](#project-components)
        - [Hardware Abstraction](#hal)
        - [State and event handling ](#event-queue-and-state-machine)
        - [Additional Components](#additional-components)



## Requirements
These are the repo requirements for development, these include cli tools, IDE, IDE extensions, etc..

- VSCode
- Platform.io extension
- Platform.io CLI tools
- Libraries used or added are installed **automatically** using the pio extnesion (No action required)

## Project Structure

### (Current)Folder structure:

### Note:
#### As of the current state of the source code:
- The ```./include``` and ```./lib``` Folders are not (yet) in use
- ```./test``` folder is planned for unit tests
- **The Entirty of the source code is in the ```./src``` folder**

### Project components

This section will NOT highlight the individual specific components but a General documentaion/guide to existing Components and/or adding new ones.

Each component's code shall be contained in a single sub-folder in the ```./src``` folder (Check existing Folders).

Let's assume the BLE Component:
The BLE component has 3 Sub folders:
- ```./BLEStackHandler``` Which contains All lib-NimBLE Callbacks
- ```./EventQueue``` Which contains the Event Queue implementation
- ```./HAL``` which contains the Hardware abstraction layer

Alongside the 3 sub-folers ther is also a BLEEvents.hpp and BluetoothState.hpp/.cpp which defines the set of Events that can occur/get triggered affecting the State Machine that handles all BLE Logic, and the Actual Sate Machine itself respectively

#### HAL
The HAL Contains all Hardware abstraction and should expose an API that allows single high level actions that the hardware is capable of with out exposing any of the underlying logic (whitelist, authentication, servers, advertisers, etc..) an instead expose a function like ```upsertAndNotifyCharacteristic()``` that handles all underlying logic and does that.

##### Note:
In some other cases especially when the component being developed does not map to a physical hardware component the name should be changed to (Initial char of the component)/AL. Example: ML predictor has the folder MAL which stands for **M**odel **A**bstraction **L**ayer.

#### Event Queue and State Machine
Each component should have it's own State Machine that handles the component's High level state, event Queue to handle Triggered events and the Event definitions them self preferably defined as an ``` Enum class{};```

Since there exists a HAL; then, Hardware specific calls inside Event Queue handlers, State machine logic, etc... is not really the norm (acceptable for testing but refactoring into it's possible place is generally preffered)

#### Additional Components
If the component being developed needs any component specific sub-component that is required for Callback definitions, external communication, etc.. a subfolder shall be created for the sub-components.



Please reach out if any of this makes no sense (It did not make a single bit of sense when I started research abt any of this)

Also do reach out if this general structure is not the best, unoptimized, has bad practices, etc... 