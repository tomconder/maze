# maze icon

Icon by [Lorc](https://lorcblog.blogspot.com/) under [CC BY 3.0](http://creativecommons.org/licenses/by/3.0/)

See https://game-icons.net/1x1/lorc/maze.html

## Creating Icon for Windows

Use cups to install `ImageMagick`

```text
choco install imagemagick
```

Use magick to create a ico file

```text
"c:\Program Files\ImageMagick-7.1.0-Q16-HDRI\magick.exe" icon-16.png icon-32.png icon-128.png icon-256.png maze.ico
```

## Creating Icon for Mac

Use `iconutils` that is provided by Apple.

```text
iconutil --convert icns --out maze.icns maze.iconset
```
