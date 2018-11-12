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
The project already uses es6 imports, so let's make sure our generated code also uses es6 imports by changing the bsconfig `module` mode to `"es6"`.

📄 bsconfig.json
```diff
      "subdirs" : true
    },
    "package-specs": {
-     "module": "commonjs",
+     "module": "es6",
      "in-source": true
    },
    "suffix": ".bs.js",

```
## Converting a utility module

The easiest way to begin introducing Reason is to convert utility modules, keeping their external interface the same.

Here, we're starting with our `agent.js` file.

We can begin by pulling out the `superagent` imports into our `agent.re` file.


### Abstract types

Abstract types, or types that don't have any definition, will often be used when we are working with external APIs. In this commit, we've defined abstract types for the global `Promise` namespace (`type globalPromise`), the `superagent` namespace that we import and the superagent `client` returned by combining the `superagent` and `globalPromise` namespaces with the `superagentPromise` module.

I like to think of an abstract type as "This thing exists but I don't care what it is".

📄 src/agent.re
```re
type superagent;
type client;
type globalPromise;

```
### Externals

Externals are the way we specify a non-Reason interface and tell the compiler what the inputs and outputs will be. Every `external` is erased at compile-time and get converted to plain JavaScript calls in the output.

BuckleScript provides a variety of attributes to add to `external` declarations, including `[@bs.module]` for importing JavaScript modules, `[@bs.val]` for referencing a static value or method, and `[@bs.scope]` for working with dot-separated namespaces.

The syntax for externals looks like this `[attributes] external functionName: (firstArgumentType, secondArgumentType) => returnType = "javascriptNameToBindTo"`.
If you are binding to a value, it looks like `[attributes] external valueName: valueType = "javascriptNameToBindTo".
If you are binding to a module, you can use `[@bs.module] external defaultImportName: importType = "module-name".

📄 src/agent.re
```diff
  type superagent;
  type client;
  type globalPromise;
+ 
+ [@bs.scope "global"] [@bs.val] external promise: globalPromise = "Promise";
+ 
+ [@bs.module]
+ external superagentPromise: (superagent, globalPromise) => client =
+   "superagent-promise";
+ [@bs.module] external superagent_: superagent = "superagent";

```
### Combining everything

Now that we have some abstract types and externals to bind our modules, we can combine everything into an export. If we don't add a `let valueName =` statement, our file will be empty because we've only defined externals that all get erased at compile time.

Because we defined all our APIs with abstract types, we can glue everything together and assign to our `superagent` variable. By default, all variables defined by `let` will be exported from the generated module.

📄 src/agent.re
```diff
  external superagentPromise: (superagent, globalPromise) => client =
    "superagent-promise";
  [@bs.module] external superagent_: superagent = "superagent";
+ 
+ let superagent = superagentPromise(superagent_, promise);

```
## Ignore

At this point, you're probably noticing a few generated items, including:
* a `.merlin` file
* a `lib/` directory
* a `src/agent.bs.js` file

Let's gitignore all these now.

📄 .gitignore
```diff
  .DS_Store
  .env
  npm-debug.log
- .idea+ .idea
+ 
+ # ReasonML
+ .merlin
+ lib/
+ *.bs.js

```
## Use our Reason code from JS

Since our `agent.re` file contains the same code as the top of our `agent.js` file, we can replace those lines with an `import` of our generated code. Notice the `.bs.js` in the import, that avoids node's resolution strategy and will load our generated code.

📄 src/agent.js
```diff
- import superagentPromise from 'superagent-promise';
- import _superagent from 'superagent';
- 
- const superagent = superagentPromise(_superagent, global.Promise);
+ import { superagent } from "./agent.bs.js";
  
  const API_ROOT = 'https://conduit.productionready.io/api';
  

```
## Convert methods

As previously mentioned, any defined variables or methods will be exported by default. This allows us to pull individual methods into our Reason code and then import them into our JS.

Here, we pull the `responseBody` method into our Reason code. By using the `##body` syntax, Reason will infer that the argument to the function is of type `Js.t(< body: 'c, .. >)` which really just means that it must receive a JS object with the property `body` that has a value of any type.

We then import the `responseBody` method into our JS code and continue to use it the same way as before. If you haven't done so yet, now would be a good time to review the generated file `agent.bs.js` to see how all of this is compiling out. A goal of BuckleScript is to have readable output so hopefully you'll be able to mentally translate the output to your Reason code.

📄 src/agent.js
```diff
- import { superagent } from "./agent.bs.js";
+ import { superagent, responseBody } from "./agent.bs.js";
  
  const API_ROOT = 'https://conduit.productionready.io/api';
  
  const encode = encodeURIComponent;
- const responseBody = res => res.body;
  
  let token = null;
  const tokenPlugin = req => {

```
📄 src/agent.re
```diff
  [@bs.module] external superagent_: superagent = "superagent";
  
  let superagent = superagentPromise(superagent_, promise);
+ 
+ let responseBody = res => res##body;

```
Moving `tokenPlugin` and `setToken` into our Reason code showcases optional values and mutations.

In Reason, we use "optional" values instead of `null` or `undefined` and the compiler will ensure we check whether we have `Some(data)` or `None`. When the application starts, we have `None` but what's this `ref()` thing? A `ref` is a language feature that allows us to create mutable variables.

We create the `token` variable as a mutable variable that starts as a `None` value.

Then, we need to rewrite the `tokenPlugin` a little bit. We change the if-statement to a `switch` that matches on `token^`. The caret after `token` means that we want to access the mutable value (it "unwraps" it from the `ref`). Our `switch` then compares the value to `Some(token)` or `None`, and if we have a token, it calls the `set` method on the `req` argument. If `token` is `None`, we need the statement `()` which is Reason's "non-value", usually called `unit` - this is because every code branch must return something and Reason has implicit returns.

Since we have a mutable value inside our Reason code, we also need to move the `setToken` method from our JS code into our Reason code. This method will be using the `:=` operator to set a mutable value. You'll also notice that we wrap the passed `_token` in `Some` to indicate that we now have some data assigned as `token`.

Once all that is done, you can import the `tokenPlugin` and `setToken` functions into your JS module and replace the JS implementations.

📄 src/agent.js
```diff
- import { superagent, responseBody } from "./agent.bs.js";
+ import { superagent, responseBody, tokenPlugin, setToken } from "./agent.bs.js";
  
  const API_ROOT = 'https://conduit.productionready.io/api';
  
  const encode = encodeURIComponent;
  
- let token = null;
- const tokenPlugin = req => {
-   if (token) {
-     req.set('authorization', `Token ${token}`);
-   }
- }
- 
  const requests = {
    del: url =>
      superagent.del(`${API_ROOT}${url}`).use(tokenPlugin).then(responseBody),
    Comments,
    Profile,
    Tags,
-   setToken: _token => { token = _token; }
+   setToken,
  };

```
📄 src/agent.re
```diff
  let superagent = superagentPromise(superagent_, promise);
  
  let responseBody = res => res##body;
+ 
+ let token = ref(None);
+ let tokenPlugin = req =>
+   switch (token^) {
+   | Some(token) => req##set("authorization", "Token " ++ token)
+   | None => ()
+   };
+ 
+ let setToken = _token => token := Some(_token);

```