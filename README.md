# devsink

Sink devices for the testbed.
Sink devices are devices that sink data (i.e SLM, DM, etc).
A devsink program interfaces the hardware device and pushes images from the shared memory file to the hardware device.
All sink device shared memory file headers contain the following keywords.

### Keywords

| Keyword  | Type   | Python type | Description                       |
| -------- | ------ | ----------- | --------------------------------- |
| KIND     | STRING | str         | Device kind (`DM`/`SLM`)          |
| SN       | STRING | str         | Serial number (8 char value)      |
| PXMAX    | DOUBLE | float       | Actuator max                      |
| FULL.W   | LONG   | int         | Modulator width (px)              |
| FULL.H   | LONG   | int         | Modulator height (px)             |
| PORT     | LONG   | int         | Link port (-1 if no control link) |
| RADMAX   | LONG   | int         | Maximum Radius (px)               |
| RADIUS   | DOUBLE | float       | Radius (px)                       |
| CENTER.X | DOUBLE | float       | Center x (px)                     |
| CENTER.Y | DOUBLE | float       | Center y (px)                     |
| FRMRATE  | DOUBLE | float       | Frame rate (fps)                  |

A devsink program also establishes a control link (a zmq interface) to send/receive additional (non-image) control data to/from the hardware.
Control data is sent as [TOML][1] and varies for each device.
The `sync` message will show the available control data.

Example transactions:


<table>
<tr>
<th>Send</th>
<th>Receive</th>
</tr>
<tr>
<td>

```toml
settings= "sync"
```
</td>
<td>

```toml
[settings]
radius = 35.0
[settings.center]
y = 200.0
x = 200.0
```
</td>
</tr>
<tr>
<td>

```toml
[settings]
radius = 100
```
</td>
<td>

```toml
[settings]
radius = 100
```
</td>
</tr>
<tr>
<td>

```toml
[settings.nudge]
x = 0
y = 1
```
</td>
<td>

```toml
[settings.center]
y = 200.0
x = 201.0
```
</td>
</tr>

</table> 
Invalid control data should be ignored without crashing.
This repository includes two sink device implementations.

- stbsink: Dummy modulator data sink.
- lcdsink: LCD SLM data sink.

## Instructions

#### build
```bash
devsink/src$ meson setup builddir
devsink/src$ meson compile -C builddir/
```
#### run
```bash
devsink/src$ ./builddir/stbsource/stbsink
devsink/src$ ./builddir/stbsource/lcdsink
```

[1]: https://en.wikipedia.org/wiki/TOML