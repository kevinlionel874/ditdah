# The DitDah case

A 3D-printed box, printed in two parts (base and lid). The circuit stays on a half-size breadboard inside the box, so nothing needs soldering when it goes in.

- **Model it** in Tinkercad at home (about 1.5 hours, Nov 28).
- **Print it** at the makerspace on Mon Nov 30 (base about 5 hours, lid about 2.5 hours at 0.2 mm layers and 15% infill). Start the base first thing in the morning.
- **Assemble** the same evening.

## Measure first

Every hole below assumes typical parts. Measure yours with calipers (or a ruler) and change the numbers before printing.

| Part | Measure | Assumed | Yours |
|---|---|---|---|
| 30mm button | Diameter of the part that goes through the panel | 28.5 mm hole | |
| 30mm button | Depth below the panel, including the microswitch | ≤ 38 mm | |
| OLED module | Size of the bright display area | 31 × 17 mm window | |
| OLED module | Board size | 36 × 34 mm | |
| 3.5mm jack | Diameter of the round barrel where the plug goes in | 6.5 mm hole | |
| Breadboard inside | Length × width × height | 82 × 55 × 9 mm (half size) | |
| Micro-USB plug | Width × height of the plug's plastic body | 14 × 12 mm opening | |
| Silicone tube | Outside diameter | 5 mm hole (tube is 4 mm) | |

If you only have the long breadboard (165 × 55 mm), make the box 180 mm wide instead of 130 and move nothing else.

## The box

Outside: **130 mm wide × 110 mm deep × 50 mm tall**. Walls 2 mm, floor 2 mm. The front is the side nearest the user.

Inside, the breadboard sits against the **left** wall at the **back**, with the Blue Pill at its left end so its USB port lines up with the left-wall opening. The front half stays clear for the button bodies, which hang about 35 mm below the lid.

### Lid (3 mm thick), seen from above

Positions are the **centers**, measured from the lid's **left** edge (x) and **back** edge (y).

| Feature | Size | Center (x, y) |
|---|---|---|
| OLED window | 31 × 17 mm rectangle | 40, 25 |
| Buzzer sound holes | 5 holes, 2 mm, in a plus shape 4 mm apart | 100, 25 |
| Dit button | 28.5 mm hole | 25, 85 |
| Dah button | 28.5 mm hole | 65, 85 |
| Mode button | 28.5 mm hole | 105, 85 |

Under the lid:

- A 2 mm tall lip, 1.5 mm thick, set in 2.3 mm from every edge, so the lid presses into the base.
- The vibration motor is stuck under the lid at (45, 85), between Dit and Dah, where the fingers rest.
- The OLED is held under its window with hot glue or double-sided tape.

### Back wall

Three 3.5mm jack holes, 6.5 mm, **18 mm above the bottom**, at x = 45, 65 and 85 mm. Seen from behind, they are J1, J2, J3 from left to right. Label them SWITCH, DIT, DAH with stickers.

### Left wall

USB opening, 14 × 12 mm, centered **31.5 mm from the back** and **17 mm above the bottom**.

### Right wall

Breath tube hole, 5 mm, **40 mm from the back** and **25 mm above the bottom**. The tube goes through and onto the MPRLS port inside.

## Modeling it in Tinkercad

1. **Base:** drag in a **Box**, set it to 130 × 110 × 50. Drag in a second Box, set it to 126 × 106 × 50, make it a **Hole**, raise it 2 mm, and align both centered (Align tool). Select both → **Group**. You have an open box with 2 mm walls and floor.
2. **Wall holes:** add **Cylinder** holes (turn them on their side for wall holes) and a Box hole for the USB opening at the positions above. Make each one longer than the wall is thick so it cuts all the way through. Group them with the base.
3. **Lid:** a Box 130 × 110 × 3. For the lip, a Box 125.4 × 105.4 × 2 with a Box hole 122.4 × 102.4 × 2 inside it, grouped, placed centered under the lid. Group lid and lip.
4. **Lid holes:** a Box hole 31 × 17 for the OLED window, Cylinder holes 28.5 mm for the buttons, and 2 mm Cylinder holes for the buzzer, at the positions in the table. Group with the lid.
5. **Export:** select the base → Export → STL. Do the same for the lid.
6. **Print:** base with the open side up; lid with the top face down (lip up). No supports needed.

## Assembly

1. Press the three 30mm buttons into the lid (Dit, Dah, Mode from left to right, seen from the front).
2. Push the three jacks through the back wall from inside and fix them with hot glue.
3. Put the breadboard in, back left, with the Blue Pill's USB at the left opening.
4. Glue the OLED under its window and the motor under the lid between Dit and Dah.
5. Push the tube through the right wall onto the MPRLS port.
6. Plug every jumper into the breadboard as in `docs/wiring.md`, close the lid, and test every input before filming.
