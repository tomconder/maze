## Generating fonts

```
npm install msdf-bmfont-xml
npx msdf-bmfont --reuse -o ./fonts.png -m "484,484" -s 42 -t msdf -f txt ./league-gothic-font.ttf
npx msdf-bmfont -u ./fonts.cfg -o ./fonts.png -s 42 -t msdf -f txt ./inter.ttf
npx msdf-bmfont -u ./fonts.cfg -o ./fonts.png -s 42 -t msdf -f txt ./inter-bold.ttf
```

Replace the `msdfgen` executable found in `tools\fonts\node_modules\msdf-bmfont-xml\bin` with
the [latest version from
github](https://github.com/Chlumsky/msdfgen/releases).

### Fonts

* [inter](https://github.com/rsms/inter) _by [Rasmus Andersson](https://github.com/rsms)_
* [league-gothic](https://github.com/theleagueof/league-gothic)
  _by [Caroline Hadilaksono](https://www.hadilaksono.com/), [Micah Rich](https://micahrich.com/), & [Tyler Finck](https://www.tylerfinck.com/)_
