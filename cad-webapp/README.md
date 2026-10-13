# Dryad CAD

Small local webapp that generates the 3D-printable body of Project Dryad.

```bash
cd cad-webapp
npm install
npm start          # http://localhost:8080
npm test           # headless smoke test of the CAD core
```

Set the **vase Ø** and **bottle Ø**, orbit the preview, then download STLs
(assembly, one-plate print layout, or each part in print orientation).

## Parts

| Part | Depends on | Notes |
|------|-----------|-------|
| Housing | nothing | Rounded pod for the **S2 Mini combo**: 10 mm corner radius and a 9 mm crown, about 76×42×73 mm. Bay holds a LOLIN ESP32-S2 Mini (34.3×25.4 mm), TP4056 USB-C charger, mini L298 (24.7×21 mm), VTC6 18650 and the diaphragm pump. Side cut-outs: USB-C (the charger) and the on/off switch. Pump inlet leaves the right wall; outlet leaves the crown into the arm |
| Front lid | screen on/off | Checkbox in the side panel. On: rounded screen window and a shallow bezel. Off: a closed lid with no aperture. The switch and the charging port stay on the side wall either way |
| Vase collar | **vase Ø** | Split C-clamp (M4 bolt + nut trap) on a flat pad that screws to the housing back |
| Sensor arm | nothing | Slim rounded bar (6.8 mm thick, 1.2 mm clip wall) with a sheath for the soil probe. The old holder was a 26×10 mm bar |
| Bottle cradle | **bottle Ø** | Cup for a ~100 ml bottle, front slit for odd shapes, tube notch for the pump inlet |

Collar, arm and cradle screw on from inside the housing (M3); the front panel
screws on from the front; the collar clamp uses an M4 bolt. The translucent
coloured bodies in the preview are stand-ins for the hardware and are not exported.

### Default combo (what the bay is sized for)

| Part | Envelope used | Where the number came from |
|------|----------------|----------------------------|
| ESP32-S2 Mini V1.0.0 | 34.3 × 25.4 × 5.0 mm | Footprint from the Wemos S2 Mini dimension drawing. The CoThings page has no millimetres. 5.0 mm thickness is assumed (PCB + USB-C). Headers in the photo are unsoldered |
| TP4056/TC4056 USB-C | 28 × 17 × 4.2 mm | Assumed. The page has no millimetres. USB-C is on a short edge and meets the left-wall port |
| L298 2-motor module | 24.7 × 21 × 12 mm | Footprint **measured** on the page (24.7 × 21 × 5 mm). 12 mm height is assumed so the 100 µF cans in the photo fit. This is not the 43 mm heatsink module |
| 18650 VTC6 | 18.35 × 65.1 mm | Assumed standard cell. The page only says format 18650. About 1 mm radial clearance and a short spring gap at each end |
| Mini pump (JSB1523078) | 40 × 22 × 20 mm | Assumed from the product photo (round 030 can, white twin-nozzle head). The page gives 5 V, 80 mA, 60 kPa, −40 kPa, 0.6 L/min, 15 g, and no size |
| Soil probe | blade 92.1 × 23.5 × 1.6 mm, head 6.5 mm | Listing **measured** 92.1 × 23.5 × 6.5 mm. The photo is a thin YL-69 fork plus a YL-38 board, so the sheath grips the blade and the 6.5 mm head sits above the clip |

There is no separate 3.7→5 V module and no 72 mm DevKit pocket in this default.
The L298 listing is rated 2–10 V and the pump label is a 3 V diaphragm pump.

`src/cad.js` is the whole model (runs in the browser and in Node);
`layout()` holds the component positions.

## Desktop AppImage

```bash
./packaging/build-appimage.sh
./dist/DryadCAD-x86_64.AppImage
```

The image bundles Node and starts the generator on http://127.0.0.1:8080, then opens it in a browser.
