# Personal Web Page


## Start the server

``` sh
$ make -C server
$ server/main
```

Serving port `18080`.


## Development

Please edit the `static/style_config.css` file.
``` sh
$ cd static
$ npx @tailwindcss/cli -i style_config.css -o style.css --watch
```

## Features

Any file in the static repository is served by the `/static/<file>` route.
