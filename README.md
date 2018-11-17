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
## Method chaining

Next, we'll begin converting the "requests" helpers as defined in `agent.js`.

But first, we'll pull in the `API_ROOT` constant. Reason reserves leading uppercase characters for Variant constructors so `API_ROOT` is renamed to `apiRoot` in `agent.re`.

📄 src/agent.re
```diff
  
  let superagent = superagentPromise(superagent_, promise);
  
+ let apiRoot = "https://conduit.productionready.io/api";
+ 
  let responseBody = res => res##body;
  
  let token = ref(None);

```
The superagent API is what people refer to as a "chaining" API - you've probably noticed the `.get().use().then()` syntax in the `agent.js` file.

We can represent these types of APIs in Reason using the `[@bs.send]` attribute combined with the Fast Pipe (`->`) operator. Fast Pipe inserts the results of a statement as the first argument to the next function in the chain. A "chaining" API will always return itself, which is `client` in our case.

For these externals, the empty string at the end indicates the method being called has the same name as our external. Note that `then` is a reserved word so we renamed it to `then_` and had to indicate the JS name is `"then"`.

In our `use` and `then_` externals, we've taken a shortcut by using generics (`'request` and `'response`) to indicate that the functions can receive "any" value.

The `then_` method will return a `Js.Promise.t(Js.Json.t)` value, which maps to a JS Promise containing some JSON value.

When we combine these externals with `->`, we get a Reason API that looks very similar to the JS chaining API.

📄 src/agent.re
```diff
    };
  
  let setToken = _token => token := Some(_token);
+ 
+ [@bs.send] external get: (client, string) => client = "";
+ [@bs.send] external use: (client, 'request => unit) => client = "";
+ [@bs.send]
+ external then_: (client, 'response => Js.Json.t) => Js.Promise.t(Js.Json.t) =
+   "then";
+ 
+ let requestGet = url =>
+   superagent->get(apiRoot ++ url)->use(tokenPlugin)->then_(responseBody);

```
We can just drop the `requestGet` method into our JavaScript file and it should work the same as the previous API. If you check out the `agent.bs.js` file, you'll notice the generated output is exatly the same as the line we are replacing.

📄 src/agent.js
```diff
- import { superagent, responseBody, tokenPlugin, setToken } from "./agent.bs.js";
+ import { superagent, requestGet, responseBody, tokenPlugin, setToken } from "./agent.bs.js";
  
  const API_ROOT = 'https://conduit.productionready.io/api';
  
  const requests = {
    del: url =>
      superagent.del(`${API_ROOT}${url}`).use(tokenPlugin).then(responseBody),
-   get: url =>
-     superagent.get(`${API_ROOT}${url}`).use(tokenPlugin).then(responseBody),
+   get: requestGet,
    put: (url, body) =>
      superagent.put(`${API_ROOT}${url}`, body).use(tokenPlugin).then(responseBody),
    post: (url, body) =>

```
Using the same techniques as above, we can translate the `del`, `put`, and `post` methods on our `requests` object. Both `put` and `post` use a generic `'body` type - though we could use a `Js.Json.t` type, this should make it easier for us to translate the rest of the file.

📄 src/agent.re
```diff
  let setToken = _token => token := Some(_token);
  
  [@bs.send] external get: (client, string) => client = "";
+ [@bs.send] external del: (client, string) => client = "";
+ [@bs.send] external put: (client, string, 'body) => client = "";
+ [@bs.send] external post: (client, string, 'body) => client = "";
  [@bs.send] external use: (client, 'request => unit) => client = "";
  [@bs.send]
  external then_: (client, 'response => Js.Json.t) => Js.Promise.t(Js.Json.t) =
  
  let requestGet = url =>
    superagent->get(apiRoot ++ url)->use(tokenPlugin)->then_(responseBody);
+ 
+ let requestDel = url =>
+   superagent->del(apiRoot ++ url)->use(tokenPlugin)->then_(responseBody);
+ 
+ let requestPut = (url, body) =>
+   superagent
+   ->put(apiRoot ++ url, body)
+   ->use(tokenPlugin)
+   ->then_(responseBody);
+ 
+ let requestPost = (url, body) =>
+   superagent
+   ->post(apiRoot ++ url, body)
+   ->use(tokenPlugin)
+   ->then_(responseBody);

```
Now, we can swap out our implementations of `del`, `put`, and `post` and clean up some (now) unused values.

📄 src/agent.js
```diff
- import { superagent, requestGet, responseBody, tokenPlugin, setToken } from "./agent.bs.js";
- 
- const API_ROOT = 'https://conduit.productionready.io/api';
+ import { requestGet, requestDel, requestPut, requestPost, setToken } from "./agent.bs.js";
  
  const encode = encodeURIComponent;
  
  const requests = {
-   del: url =>
-     superagent.del(`${API_ROOT}${url}`).use(tokenPlugin).then(responseBody),
+   del: requestDel,
    get: requestGet,
-   put: (url, body) =>
-     superagent.put(`${API_ROOT}${url}`, body).use(tokenPlugin).then(responseBody),
-   post: (url, body) =>
-     superagent.post(`${API_ROOT}${url}`, body).use(tokenPlugin).then(responseBody)
+   put: requestPut,
+   post: requestPost,
  };
  
  const Auth = {

```
The `requests` object can be removed and we can use our Reason methods directly.

📄 src/agent.js
```diff
  
  const encode = encodeURIComponent;
  
- const requests = {
-   del: requestDel,
-   get: requestGet,
-   put: requestPut,
-   post: requestPost,
- };
- 
  const Auth = {
    current: () =>
-     requests.get('/user'),
+     requestGet('/user'),
    login: (email, password) =>
-     requests.post('/users/login', { user: { email, password } }),
+     requestPost('/users/login', { user: { email, password } }),
    register: (username, email, password) =>
-     requests.post('/users', { user: { username, email, password } }),
+     requestPost('/users', { user: { username, email, password } }),
    save: user =>
-     requests.put('/user', { user })
+     requestPut('/user', { user })
  };
  
  const Tags = {
-   getAll: () => requests.get('/tags')
+   getAll: () => requestGet('/tags')
  };
  
  const limit = (count, p) => `limit=${count}&offset=${p ? p * count : 0}`;
  const omitSlug = article => Object.assign({}, article, { slug: undefined })
  const Articles = {
    all: page =>
-     requests.get(`/articles?${limit(10, page)}`),
+     requestGet(`/articles?${limit(10, page)}`),
    byAuthor: (author, page) =>
-     requests.get(`/articles?author=${encode(author)}&${limit(5, page)}`),
+     requestGet(`/articles?author=${encode(author)}&${limit(5, page)}`),
    byTag: (tag, page) =>
-     requests.get(`/articles?tag=${encode(tag)}&${limit(10, page)}`),
+     requestGet(`/articles?tag=${encode(tag)}&${limit(10, page)}`),
    del: slug =>
-     requests.del(`/articles/${slug}`),
+     requestDel(`/articles/${slug}`),
    favorite: slug =>
-     requests.post(`/articles/${slug}/favorite`),
+     requestPost(`/articles/${slug}/favorite`),
    favoritedBy: (author, page) =>
-     requests.get(`/articles?favorited=${encode(author)}&${limit(5, page)}`),
+     requestGet(`/articles?favorited=${encode(author)}&${limit(5, page)}`),
    feed: () =>
-     requests.get('/articles/feed?limit=10&offset=0'),
+     requestGet('/articles/feed?limit=10&offset=0'),
    get: slug =>
-     requests.get(`/articles/${slug}`),
+     requestGet(`/articles/${slug}`),
    unfavorite: slug =>
-     requests.del(`/articles/${slug}/favorite`),
+     requestDel(`/articles/${slug}/favorite`),
    update: article =>
-     requests.put(`/articles/${article.slug}`, { article: omitSlug(article) }),
+     requestPut(`/articles/${article.slug}`, { article: omitSlug(article) }),
    create: article =>
-     requests.post('/articles', { article })
+     requestPost('/articles', { article })
  };
  
  const Comments = {
    create: (slug, comment) =>
-     requests.post(`/articles/${slug}/comments`, { comment }),
+     requestPost(`/articles/${slug}/comments`, { comment }),
    delete: (slug, commentId) =>
-     requests.del(`/articles/${slug}/comments/${commentId}`),
+     requestDel(`/articles/${slug}/comments/${commentId}`),
    forArticle: slug =>
-     requests.get(`/articles/${slug}/comments`)
+     requestGet(`/articles/${slug}/comments`)
  };
  
  const Profile = {
    follow: username =>
-     requests.post(`/profiles/${username}/follow`),
+     requestPost(`/profiles/${username}/follow`),
    get: username =>
-     requests.get(`/profiles/${username}`),
+     requestGet(`/profiles/${username}`),
    unfollow: username =>
-     requests.del(`/profiles/${username}/follow`)
+     requestDel(`/profiles/${username}/follow`)
  };
  
  export default {

```
We're not going to finish converting the rest of this file, partly because the namespacing patterns aren't easy in Reason and partly because we want to talk about co-location refactors later.


## ReasonReact

Let's change our focus away from utility functions and look at how we can migrate React components written in JS over to Reason. We can do this using ReasonReact, which also provides some helpful utilities (like `reducerComponent`) to simplify our application.

First, we'll change some of our JS patterns to improve our Reason conversion. Instead of passing the `Article` component to ReactRouter, we'll use a `render` function so we can pass `currentUser` directly to the component as a prop instead of plucking it out of the Redux store inside that component. We then remove those property mappers from our `Article` component. Side note: we should probably do this for all our components but we didn't want to pollute the diff.

📄 src/components/App.js
```diff
              <Route path="/register" component={Register} />
              <Route path="/editor/:slug" component={Editor} />
              <Route path="/editor" component={Editor} />
-             <Route path="/article/:id" component={Article} />
+             <Route path="/article/:id" render={(props) => <Article {...props} currentUser={this.props.currentUser} />} />
              <Route path="/settings" component={Settings} />
              <Route path="/@:username/favorites" component={ProfileFavorites} />
              <Route path="/@:username" component={Profile} />

```
📄 src/components/Article/index.js
```diff
  
  const mapStateToProps = state => ({
    ...state.article,
-   currentUser: state.common.currentUser
  });
  
  const mapDispatchToProps = dispatch => ({

```
Now, we'll add `reason-react` as a dependency using the `npm install -S reason-react` command. Whenever you install a Reason dependency, you must add it to the `bs-dependencies` array in your `bsconfig.json` file so the build system knows which modules to build.

For JSX support, we also add the `react-jsx` transform in our `bsconfig.json` file.

📄 bsconfig.json
```diff
    "suffix": ".bs.js",
    "bs-dependencies": [
      // add your dependencies here. You'd usually install them normally through `npm install my-dependency`. If my-dependency has a bsconfig.json too, then everything will work seamlessly.
+     "reason-react"
    ],
    "warnings": {
      "error": "+101"
    },
    "namespace": true,
-   "refmt": 3
+   "refmt": 3,
+   "reason": {
+     "react-jsx": 2
+   },
  }

```
📄 package.json
```diff
      "react-router": "^4.1.2",
      "react-router-dom": "^4.1.2",
      "react-router-redux": "^5.0.0-alpha.6",
+     "reason-react": "^0.5.3",
      "redux": "^3.6.0",
      "redux-devtools-extension": "^2.13.2",
      "redux-logger": "^3.0.1",

```
We'll start by converting a pretty straight-forward, stateless component to Reason. Since our component is stateless, we'll construct our default component with `ReasonReact.statelessComponent()`. The only argument this method needs is a string to use as a debug name - a handy trick is to always use `__MODULE__` which Reason injects as the current filename (or module name if using nested modules).

In Reason, JSX compiles to the `make` method of a module, so we'll define a `make` that returns our ReasonReact component. All `props` are passed as labeled arguments, so here we define `~comments`, `~slug` and `~currentUser` - all of which are required (an optional property would look like `~token=?`). The only positional argument is `children` passed to the component, but in this case we define it as `_children` because we don't actually accept any children.

For the return value, we return a record that spreads the default `component` and then we override the `render` method. `render` receives one argument `self` - sort of like `this` in JS - but we aren't using it so we named it `_self`. This is a common pattern in Reason and reduces compiler warnings about unused variables.

Our `render` currently just outputs an empty `<div>` - but we'll be adding the rest from the original module next.

📄 src/components/Article/CommentList.re
```re
let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~comments, ~slug, ~currentUser, _children) => {
  ...component,
  render: _self =>
    <div>
    </div>
};

```
While converting our CommentList component, we encounter another custom component - `Comment` - but that's fine because ReasonReact allows us to wrap JS components for use in Reason.

The `make` function for that component becomes a passthrough to `ReasonReact.wrapJsForReason()`, which takes:
* A `ReasonReact.reactClass` as `~reactClass` - this will come from an `external` declaration
* The JS props as `~props` which needs to be JS serializable
* The children to pass - we use empty arrays for both of these but you can pass through `children` if your components accept children

📄 src/components/Article/CommentList.re
```diff
+ module Comment = {
+   [@bs.module "./Comment.js"]
+   external reactClass: ReasonReact.reactClass = "default";
+ 
+   let make = (~comment, ~currentUser, ~slug, _children) =>
+     ReasonReact.wrapJsForReason(
+       ~reactClass,
+       ~props={"comment": comment, "currentUser": currentUser, "slug": slug},
+       [||],
+     );
+ };
+ 
  let component = ReasonReact.statelessComponent(__MODULE__);
  
  let make = (~comments, ~slug, ~currentUser, _children) => {

```
Now that we have a wrapper for our external React component, we can utilize Belt's `Array.map` function to map all our comments to the Comment component.

Notice the `...{ }` around our map statement, the `{ }` allows us to inline statements and the `...` spreads an array as children in a DOM structure. You'll see and use this pattern a lot.

📄 src/components/Article/CommentList.re
```diff
    ...component,
    render: _self =>
      <div>
-     </div>
+       ...{
+            Belt.Array.map(comments, comment =>
+              <Comment comment currentUser slug key=comment##id />
+            )
+          }
+     </div>,
  };

```
To use our ReasonReact component in JS, we need to utilize the `ReasonReact.wrapReasonForJS()` API and export it as `default`.

This API takes:
* The Reason component you've created as `~component`
* A function that maps JS props to your component's `make` method while passing in the correctly converted parameters

📄 src/components/Article/CommentList.re
```diff
           }
      </div>,
  };
+ 
+ let default =
+   ReasonReact.wrapReasonForJs(~component, props =>
+     make(
+       ~comments=props##comments,
+       ~slug=props##slug,
+       ~currentUser=props##currentUser,
+       [||],
+     )
+   );

```
We can now import our `CommentList.bs.js` file and remove the old `CommentList.js` file. Usage remains the same and our application should function correctly.

If you have the React-DevTools installed, you'll be able to see the component named `CommentList-MigrateRealworldWorkshop`.

📄 src/components/Article/CommentContainer.js
```diff
  import CommentInput from './CommentInput';
- import CommentList from './CommentList';
+ import CommentList from './CommentList.bs.js';
  import { Link } from 'react-router-dom';
  import React from 'react';
  

```
📄 src/components/Article/CommentList.js
```diff
- import Comment from './Comment';
- import React from 'react';
- 
- const CommentList = props => {
-   return (
-     <div>
-       {
-         props.comments.map(comment => {
-           return (
-             <Comment
-               comment={comment}
-               currentUser={props.currentUser}
-               slug={props.slug}
-               key={comment.id} />
-           );
-         })
-       }
-     </div>
-   );
- };
- 
- export default CommentList;

```