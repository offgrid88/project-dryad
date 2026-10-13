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
| Housing | nothing | Rounded pod: 10 mm corner radius and a 9 mm crown (about 86×43×101 mm). One pocket fits an ESP32-S3-DevKitC-1 (70×28 mm) and the smaller C6 and C3 DevKits, plus the USB-C charger, 3.7→5 V DC-DC, mini L298, battery and pump. Side cut-outs: USB-C and the on/off switch, plus the pump tubes |
| Front lid | screen on/off | Checkbox in the side panel. On: rounded screen window and a shallow bezel. Off: a closed lid with no aperture. The switch and the charging port stay on the side wall either way |
| Vase collar | **vase Ø** | Split C-clamp (M4 bolt + nut trap) on a flat pad that screws to the housing back |
| Sensor arm | nothing | Reaches over the rim; slot for capacitive sensor + tube channel for the pump outlet |
| Bottle cradle | **bottle Ø** | Cup for a ~100 ml bottle, front slit for odd shapes, tube notch for the pump inlet |

Collar, arm and cradle screw on from inside the housing (M3); the front panel
screws on from the front; the collar clamp uses an M4 bolt. Hardware sizes are
editable in the side panel and are placeholders until measured against the real
modules. The translucent coloured bodies in the preview are stand-ins for the
hardware and are not exported.

`src/cad.js` is the whole model (runs in the browser and in Node);
`layout()` holds the component positions.

## Desktop AppImage

```bash
./packaging/build-appimage.sh
./dist/DryadCAD-x86_64.AppImage
```

The image bundles Node and starts the generator on http://127.0.0.1:8080, then opens it in a browser.
