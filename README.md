# RX8 ECU ROM Dumper

This project is very much a work in progress. It's primarily developed by myself
with occasional assistance from a small group of enthusiasts in the community.
If you wish to get involved, *please* do so. 

## Disclaimer

It's not my fault if you mess your car up. Really. If you're here, I assume
you at least *sort of* understand what you're doing. It should be noted
that **THERE IS ALMOST NO MORE POWER LEFT** in the OEM tune for RX8 from
Mazda. If you are hunting for more power out of your stock Renisis, you
will not find it here. It is expected that this project is targeted
at tuners for ported, boosted or otherwise modified engines.

## Usage

Execute `ecudump` or `ecudump.exe` depending on your platform. 
It should take about 2 minutes to complete. Once complete, you
should have a file in the same directory as the executable labeled
with your ECU's VIN and CALID. If you experience problems, please
feel free to reach out or open an issue. 

## Planned Features

This project is still in it's infancy, and probably won't get a ton of
love until I have more time for it. I do have some goals and non-goals
in mind.

### Goals

* ECU flashing. currently only downloading is supported, for this to be useful it'll need to be able to flash firmware back to the ECU.
* Develop/port a kernel or bootloader. I'm not currently sure if this will be required, but if so it will get done.
* ROMRaider or ECUFlash integration. For flashing the ROM to be useful to anyone, the ROM will need to be mapped.

### Non Goals

* Make money. I started this project specifically because I disagree with the model that current companies are relying on.
* Replace Ecutek, Cobb, or whatever your favorite tuning company is. They have a business, I am just bored.
* Handhold or teach engine tuning. There are plenty of (better) resources for this already.
* Handhold or teach reverse engineering. There are not resources for this unfortunately, and this will not be a solution to that.
* New ECU features 
* Removing ECU features (yes, this includes the immobilizer. Just pay $100 for a new key)

With all that being said, I am doing this "for fun", on my own free time. If you feel so inclined buy me a coffee with the
sponsor button above. 

## Getting Involved

My long term goal is to have an option for tuning RX8s without needing to
pay $250+ for a license to closed source software on top of the closed source
J2354 Interface. With that said, if you are also interested please let me know.
There is a lot of ongoing work happening and would love some additional help.
Issues, Pull Requests, and any other feedback is welcome. 

## Linux Support

Linux is supported experimentally, however you will have to compile it yourself.
If you (the reader) are interested in this, feel free to reach out or open an issue.

```bash
# clone the repo
git clone https://github.com/ConnorRigby/rx8-ecu-dump.git -b v0.1.0
cd rx8-ecu-dump

# fetch and build external dependencies (very important)
git submodule update --init --recursive
cd lib/j2534

# this patch needs to be applied to fix a bug in the library.
git patch apply ../*.patch
cd j2534
make

# optionally install the library globally (you proably don't want/need this)
sudo make install

# build the program
cd ../../../
make
```

Once built, the same instructions should apply as for Windows. There is one caveat that
may trick Linux users up: permissions. See [the j2534 documentation](https://github.com/dschultzca/j2534/tree/bf08e0d923f0e5b28370d3ec0ed402d8093371e6#using-the-library).

## LICENSE

Code is licensed under the APACHE-2 license where possible.
other licenses may apply to individual files. Make sure to check
for this if you plan on using anything found in this repo. Notably
the code in the folder `J2534` is Copyright (C) 2004- Tactrix Inc.

## Special Thanks

* [gooflophaze](https://github.com/gooflophaze)
* [dschultzca](https://github.com/dschultzca)
* [fenugrec](https://github.com/fenugrec)
* equinox92
* [speepsio](https://github.com/speepsio)
