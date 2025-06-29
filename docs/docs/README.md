# Modifying the Documentation

This website is built using [Material for MKDocs](https://squidfunk.github.io/mkdocs-material/), a Material UI inspired
theme for MKDocs.

The content below is focused on how to modify the documentation site.

## Building docs locally

To build and test the documentation locally:

1. Navigate to the `docs` directory:
   ```bash
   cd docs
   ```

2. Install `mkdocs` and the Material theme if you haven't already:
   ```bash
   pip install mkdocs mkdocs-material
   ```

2. Run the build command:
   ```bash
   mkdocs build
   ```

This will generate a static build of the documentation site in the `site` directory. You can then serve this directory
to view the site locally using:

```bash
mkdocs serve
```
