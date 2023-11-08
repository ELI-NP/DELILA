# DELILA (Digital Extreme Light Infrastructure List-mode Acquisition)

DELILA uses [DAQ-Middleware](https://daqmw.kek.jp/) and [CAEN libraries](https://www.caen.it/subfamilies/software-libraries/) mainly, and something ([ROOT](https://root.cern/), [JSON for Modern C++](https://github.com/nlohmann/json), and more).

### Directories

- Components
- DAQ-Middleware components. Reader for CAEN digitizer, Recorder using ROOT, etc.
- DAQController
- DAQ Controller written in TypeScript with Angular.
- Controller
- Transpiled (TypeScript to JavaScript) version of DAQController. This expects to be placed **/controller** (e.g. http://192.168.1.11/controller)
- TDigiTES
- CARN digitizer handler.
- Web API (NYI)
- Some parameters and status GET and POST web server. This will include something more (plan).

### Files

- XML files
- Configuration files for DAQ-Middleware.
- conf files
- Configuration files for TDigiTES.

### Aknowledgement

- This work is carried out under the contract PN 23 21 01 06 sponsored by the Romanian Ministry of Research.
