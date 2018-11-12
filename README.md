# Migrate Realworld Workshop

The original [react-redux-realworld-example-app](https://github.com/gothinkster/react-redux-realworld-example-app) has been squashed into a single commit to streamline this workshop. We don't mean to discredit any contributors so please check the original project for the previous commit history.

The original project uses the standard `npm install` and `npm start` commands for installing dependencies and running application.


## Editor & Editor Integrations

You'll want to have [vscode](https://code.visualstudio.com/) with the [reason-vscode](https://marketplace.visualstudio.com/items?itemName=jaredly.reason-vscode) extension or [atom](https://atom.io/) with the [atom-ide-reason](https://github.com/reasonml-editor/atom-ide-reason) plugin.

You can use Vim or another editor but we suggest setting it up with [reason-language-server](https://github.com/jaredly/reason-language-server) and ymmv.


## Install BuckleScript

We'll want BuckleScript, the Reason/OCaml-to-JS compiler, installed in order to build our Reason files.

Run the command: `npm install -D bs-platform` to get the latest version and save under devDependencies in the package.json

📄 package.json
```diff
    "version": "0.1.0",
    "private": true,
    "devDependencies": {
+     "bs-platform": "^4.0.7",
      "cross-env": "^5.1.4",
      "react-scripts": "1.1.1"
    },

```
## Create a bsconfig.json file

We'll want to run the command `npx bsb -init . -theme basic-reason` which will generate a file named `bsconfig.json` in our project. The `npx` prefix will run our local version of `bsb`, which is the command line interface to BuckleScript, so we know we're using the same version to generate our config and compile our files.

This command will also generate a `.vscode/` directory and a `src/Demo.re` file, but we don't actually need those so we can delete them.

📄 bsconfig.json
```json
// This is the configuration file used by BuckleScript's build system bsb. Its documentation lives here: http://bucklescript.github.io/bucklescript/docson/#build-schema.json
// BuckleScript comes with its own parser for bsconfig.json, which is normal JSON, with the extra support of comments and trailing commas.
{
  "name": "migrate-realworld-workshop",
  "version": "0.1.0",
  "sources": {
    "dir" : "src",
    "subdirs" : true
  },
  "package-specs": {
    "module": "commonjs",
    "in-source": true
  },
  "suffix": ".bs.js",
  "bs-dependencies": [
      // add your dependencies here. You'd usually install them normally through `npm install my-dependency`. If my-dependency has a bsconfig.json too, then everything will work seamlessly.
  ],
  "warnings": {
    "error" : "+101"
  },
  "namespace": true,
  "refmt": 3
}

```