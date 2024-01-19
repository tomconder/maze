## Generating fonts

```
npm install msdf-bmfont-xml
npx msdf-bmfont --reuse -o fonts.png -m "384,384" -s 42 -t msdf -f txt path\to\league-gothic-font.ttf
msdf-bmfont -u ./fonts.cfg -o fonts.png -s 44 -t msdf -f txt path\to\inter-font.ttf
```

Replace the `msdfgen` executable found in `tools\fonts\node_modules\msdf-bmfont-xml\bin` with
the [latest version from
github](https://github.com/Chlumsky/msdfgen/releases).

Also you can fix the validation in `tools\fonts\node_modules\msdf-bmfont-xml\cli.js` by replacing `fileValidate` with
this simple function.

```
function fileValidate(filePath) {
  return path.normalize(filePath);
}
```
### Fonts

* [league-gothic](https://github.com/theleagueof/league-gothic)
  _by [Caroline Hadilaksono](https://www.hadilaksono.com/), [Micah Rich](https://micahrich.com/), & [Tyler Finck](https://www.tylerfinck.com/)_
* [inter](https://github.com/rsms/inter) _by [Rasmus Andersson](https://github.com/rsms)_