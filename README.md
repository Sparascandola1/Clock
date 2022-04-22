<div id="header" align="center">
	
# ***The Rich Purnell Manuver*** ⏰
	
  <img src="https://media.giphy.com/media/dWesBcTLavkZuG35MI/giphy.gif" width="600" height="300"/>
</div>

<div align="center">

## Introduction

  <p>This project is built using the DE-10 Standard Development kit! The board itself has several components that can be used to develop a variety of projects. Along with all the bells and whistles the DE-10 Standard has Field Programmable Gate Arrays (FPGA) that allow a developer to quickly on and off load digital circuits that can are created using Verilog and VHDL programming languages. The purpose of this project was explore the world of embedded systems in order to better understand what code is doing at the bare metal level as well as introduce myself to FPGA technology. 
  </p>
	
  <div align="center">
    <img src="https://user-images.githubusercontent.com/47187874/164385696-d92532a6-d4e0-4259-b4f0-e96995226263.png" width="800" height="500"/>
  </div>
	
## Requirements

### Functional
- [x] Display current time on the 7-segment display
- [x] Switch between all 4 standard American timezones
- [x] Program an alarm for a time of the users choice
- [x] Allow a user to set the alarm and turn it off
- [ ] Build a digital curcit in VHDL to be the 7-segment decorder 
***
### Non-Functional
- [x] Software backup to include program files and on-board OS
- [x] Fault Tolerance
	
## Technologies
| ***Choice*** | ***Reason*** | 
| :--- | :---: |
| DE-10 Standard | This board is the ultimate embedded environment while also being a great way to introduce oneself to FPGA design and implementation |
| Putty | This is third party software provides a developer with a CLI that helps interact with the boards headless OS |
| SoC FPGA Embedded Development Suite | EDS allows for ease of development on a windows machine, coming equipped to handle Linux based terminal commands|
| Quartus Prime | Quartus Prime is the tool used to create, synthesize, and on-board digital circuits created with VHDL and VERILOG  |
| FileZilla  | FZ is opensource software that makes transferring files onto the board easy and simple |
| VHDL/VERILOG/C | These are the three programming languages used to develop the clock program |
  
***
  <p> Initially the goal is to be able to build program files using a make function, from there they can be downloaded on to the board via a software program called FileZilla. Once the file is on the board it can be ran by using a Putty CLI interface. The program will wire up the 7-segment and LCD displays. The LCD will be programmed to have three different instruction sets, the first is the main instruction on how to change times zones and what to press to program and set the alarm. The second set is the instructions on how to program the alarm and these will be displayed when the user presses the first button and starts to program the alarm kicking off a new process flow. The third is meant more for reference when the user tries to do an action that they cannot i.e., set the alarm without first programing one in. Another instance where these instructions will show is if the user is trying to program the alarm and has not reset all the switches before starting. The LCD screen will also flash when the third set of instructions is displayed so the user can be alerted to the change in environment. Every time an I/O peripheral is used it will call an interrupt handler so the CPU can make the time to perform whatever action is needed at the time. The main program will be driven by several other support or utility modules that will consist of tasks related to components or tasks that would be redundant in the code. The separation of these modules will make the program easier to write but also make the code reusable in different projects. 
  </p>
	
***
	
  <p>All the technology in this project was new to me aside from the slight experience programming in the C language. I choose to dive into such a new environment for two reasons the first, I have never worked with embedded systems throughout my time in college and was curious of how one is architected and what new approaches would need to be taken to create a working project. Second is that I really enjoy facing new challenges and figuring new problems, being that the environment is so starkly different it provided more than enough of the challenge that I was looking for and really helped me to grow as a developer. C is not object orientated like Java or C# and classes don’t really exist. To create a maintainable codebase, I used modules and header files along procedural programming techniques. I taught myself to use tools like Putty, FileZilla, and Quartus Prime in the process of development to interface with the OS on the board, transfer new program files over, and create digital circuits to leverage the FPGA technology inherent on the board. 
  </p>

***
	
### Component Layout and Description

 <div align="center">
    <img src="https://user-images.githubusercontent.com/47187874/164786126-b8e3e00d-46dd-4a07-a830-04e72033169a.png" width="500" height="300"/>
  </div>
	
***
	
### Module Diagrams
	
  <p>The main module is the clock, all the others are support libraries that will be used to achieve the goal of supporting user input. Components that are involved in the process are being treated like an object in this design solution so the 7-segment display, buttons, and the LCD all have separate modules. Each component has its own set of distinct functions that are needed in the main program called Clock. The program also needs the ability to map to physical memory and use interrupts throughout programs life cycle. These to modules are more utility based than the component-based ones but all of them make up the library that the main program will use.  
  </p>
	
 <div align="center">
    <img src="https://user-images.githubusercontent.com/47187874/164787211-0a320792-482d-4d59-b8d8-0d4867fb69c2.png" width="500" height="500"/>
  </div>
	
***
	
### Flow Charts/Process Flows

  <p>After the start of the program the user has two options, one is to immediately flip switch one and start the program by displaying the local time. After which if any other of the first four switches have been flipped in conjunction to the first the time zone with change accordingly. The combination of switch one and switch two will display the Central time zone; switches one and three will display the Mountain time zone; and switches one and four will display the Pacific time zone. If the switch one is not engaged than the program will do nothing and a message to engage the switch will display and flash on the LCD screen. 
  </p>
	
 <div align="center">
    <img src="https://user-images.githubusercontent.com/47187874/164787632-32e88fee-e809-493d-a07e-4575d3b6c997.png" width="600" height="600"/>
  </div>
	
  <p>Option number two for the user is to press one of the three operational buttons. Button one will start the alarm protocol that allows a user to program an alarm. When the button is pushed the LCD screen will change to list new instructions on how to proceed with programing the alarm. The first step is too flip all the switches to the disengaged position, if this isn’t done within 20 seconds the LCD screen will flash with a message telling the user to disengage all the switches. Once the user has done that the next step is to use the ten switches to input a time that matches the time, they wish to set an alarm for. Since the clock is formatted in standard military time the need to pick between PM and AM is not required. After the user sees their desired time on the 7-segment display they will press the same button that started this process to finish it. The second button sets the alarm so it will go off but also if the alarm is already set and going off this button will instead stop the alarm. Also, if the alarm has not been programmed yet then the LCD screen will flash with a message saying to program an alarm first. The third button resets the programed time so the alarm cannot be set.
  </p>
	
 <div align="center">
    <img src="https://user-images.githubusercontent.com/47187874/164787644-428897f6-4a80-4b3d-b13e-cb93be58d416.png" width="600" height="600"/>
  </div>
	
***
	
## Risks
| ***Description*** | ***Project Impact*** | ***Action Plan*** | ***Level*** | 
| :---: | :---: | :--: | :--: |   
| Memory requirements | If the memory of the board fills up no more files can be downloaded to it. | Keep program files as small as possible. | Low |
| Cannot load cross compiler onto host machine. | If the files cannot be compiled than no program could run on the board. | Working on downloading and locating the correct directory with the cross compiler in it. | High |
| A short on the board | If the board fries the how projected is scrubbed | Have a backup board but also be careful when transporting the board and using it to not emit and static. | High |
</div>









