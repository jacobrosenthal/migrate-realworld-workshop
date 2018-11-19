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
### CommentList

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
### Link binding

Throughout the rest of our `Article` conversion, we'll be using the `Link` component a few times - so let's create it a `Link.re` file in our base `components` directory. Other Reason files can see any other Reason module within our application, so it doesn't really matter where it is.

📄 src/components/Link.re
```re
[@bs.module "react-router-dom"]
external reactClass: ReasonReact.reactClass = "Link";

let make = (~_to, ~className, children) =>
  ReasonReact.wrapJsForReason(
    ~reactClass,
    ~props={"to": _to, "className": className},
    children,
  );

```
### ArticleActions

We'll also be using our `agent` from Reason, so we can begin adding submodule namespaces to our `agent.re` file. It's a little duplication of code to start but the end result will be typed, safe, and wonderful.

📄 src/agent.re
```diff
    ->post(apiRoot ++ url, body)
    ->use(tokenPlugin)
    ->then_(responseBody);
+ 
+ module Articles = {
+   let del = slug => requestDel("/articles/" ++ slug);
+ };

```
Now that some housekeeping is done, we can convert our `ArticleActions` component.

Some interesting things of note here:
* Every child has to have the same type so we can't use plain strings inside our DOM elements and need to convert them to ReasonReact components using `ReasonReact.string()`.
* `to` is a reserved word in Reason so we named our prop `_to` on Link.
* Since `del` is defined at the top of our `make` function, it looks like our component record is "just hanging out", but we're actually defining and returning it in one statement.

📄 src/components/Article/ArticleActions.re
```re
let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~article, ~onClickDelete, ~canModify, _children) => {
  let del = _evt => onClickDelete(Agent.Articles.del(article##slug));

  {
    ...component,
    render: _self =>
      if (canModify) {
        <span>
          <Link
            _to={"/editor/" ++ article##slug}
            className="btn btn-outline-secondary btn-sm">
            <i className="ion-edit" />
            {ReasonReact.string("Edit Article")}
          </Link>
          <button className="btn btn-outline-danger btn-sm" onClick=del>
            <i className="ion-trash-a" />
            {ReasonReact.string("Delete Article")}
          </button>
        </span>;
      } else {
        <span />;
      },
  };
};

```
### ArticleMeta

Here's another fairly straightforward conversion. But we needed to change our `Link` component's `className` prop to an optional argument, which means we need to reach for the `Js.Nullable.fromOption` converter to change a `None` value into a `null` in our JS.

We also used a couple of Js.Date methods to work with a date string. Remember to wrap your strings in `ReasonReact.string`.

An extra `~onClickDelete` prop was added to this component because this application was over-abstracted and we need to thread these handlers throughout the components. If there's time later, we may show a pattern to tease apart these abstractions to make them nicer.

📄 src/components/Article/ArticleMeta.re
```re
let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~article, ~canModify, ~onClickDelete, _children) => {
  ...component,
  render: _self =>
    <div className="article-meta">
      <Link _to={"/@" ++ article##author##username}>
        <img src=article##author##image alt=article##author##username />
      </Link>
      <div className="info">
        <Link _to={"/@" ++ article##author##username} className="author">
          {ReasonReact.string(article##author##username)}
        </Link>
        <span className="date">
          {
            Js.Date.fromString(article##createdAt)
            ->Js.Date.toDateString
            ->ReasonReact.string
          }
        </span>
      </div>
      <ArticleActions canModify article onClickDelete />
    </div>,
};

```
📄 src/components/Link.re
```diff
  [@bs.module "react-router-dom"]
  external reactClass: ReasonReact.reactClass = "Link";
  
- let make = (~_to, ~className, children) =>
+ let make = (~_to, ~className=?, children) =>
    ReasonReact.wrapJsForReason(
      ~reactClass,
-     ~props={"to": _to, "className": className},
+     ~props={"to": _to, "className": Js.Nullable.fromOption(className)},
      children,
    );

```
### DeleteButton

Converting `DeleteButton` is similar to the last couple components. You'll notice `ReasonReact.null` instead of returning `null` directly - this is a correctly typed component representing an empty component.

We also duplicated another method from the `agents.js` for easier use.

📄 src/agent.re
```diff
  module Articles = {
    let del = slug => requestDel("/articles/" ++ slug);
  };
+ 
+ module Comments = {
+   let delete = (slug, commentId) =>
+     requestDel("/articles/" ++ slug ++ "/comments/" ++ commentId);
+ };

```
📄 src/components/Article/DeleteButton.re
```re
let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~onClick, ~show, ~slug, ~commentId, _children) => {
  let del = _evt => {
    let payload = Agent.Comments.delete(slug, commentId);
    onClick(payload, commentId);
  };

  {
    ...component,
    render: _self =>
      if (show) {
        <span className="mod-options">
          <i className="ion-trash-a" onClick=del />
        </span>;
      } else {
        ReasonReact.null;
      },
  };
};

```
### Comment

When converting the `Comment` component, we had to thread the `onDelete` handler through to the `DeleteButton` component. We also use a switch statement to check if we have a `currentUser` or not before comparing the username to the author's username.

📄 src/components/Article/Comment.re
```re
let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~comment, ~currentUser=?, ~slug, ~onDelete, _children) => {
  ...component,
  render: _self => {
    let show =
      switch (currentUser) {
      | Some(currentUser) =>
        currentUser##username === comment##author##username
      | None => false
      };

    <div className="card">
      <div className="card-block">
        <p className="card-text"> {ReasonReact.string(comment##body)} </p>
      </div>
      <div className="card-footer">
        <Link
          _to={"/@" ++ comment##author##username} className="comment-author">
          <img
            src=comment##author##image
            className="comment-author-img"
            alt=comment##author##username
          />
        </Link>
        {ReasonReact.string(" ")}
        <Link
          _to={"/@" ++ comment##author##username} className="comment-author">
          {ReasonReact.string(comment##author##username)}
        </Link>
        <span className="date-posted">
          {
            Js.Date.fromString(comment##createdAt)
            ->Js.Date.toDateString
            ->ReasonReact.string
          }
        </span>
        <DeleteButton show slug commentId=comment##id onClick=onDelete />
      </div>
    </div>;
  },
};

```
### CommentInput

We're finally getting into a `reducerComponent`! Before we begin, we need to define our `state` shape and the `action`s that can be taken. Remind you of anything? It's like a supercharged redux because we have a really powerful type system supporting it.

We have to define the `state` and `action` types before we call the `ReasonReact.reducerComponent` API. If we mess up the order, the compiler will let us know that they need to come first.

📄 src/components/Article/CommentInput.re
```re
type state = {body: string};

type action =
  | SetBody(string)
  | Submit;

let component = ReasonReact.reducerComponent(__MODULE__);

```
When defining our reducer component, we'll first need an `initialState` method to return a record that matches our `state` type above.

📄 src/components/Article/CommentInput.re
```diff
    | Submit;
  
  let component = ReasonReact.reducerComponent(__MODULE__);
+ 
+ let make = (_children) => {
+   ...component,
+   initialState: () => {body: ""},
+ };

```
Next, we need a `reducer` method that receives an `action` taken and the current `state`. For this component, when we receive a `SetBody` action, we set the body to the payload of our action using the `ReasonReact.Update` API.

When we receive a `Submit` action, we use the `ReasonReact.UpdateWithSideEffects` API to reset the `body` property and trigger a side effect that creates the comment and calls the `onSubmit` prop. Notice that we send `state.body` in the API call, which was the previous body before the record was updated (because immutability!!!).

We also needed to duplicate another method from `agent.js`.

📄 src/agent.re
```diff
  };
  
  module Comments = {
+   let create = (slug, comment) =>
+     requestPost("/articles/" ++ slug ++ "/comments", {"comment": comment});
+ 
    let delete = (slug, commentId) =>
      requestDel("/articles/" ++ slug ++ "/comments/" ++ commentId);
  };

```
📄 src/components/Article/CommentInput.re
```diff
  
  let component = ReasonReact.reducerComponent(__MODULE__);
  
- let make = (_children) => {
+ let make = (~slug, ~onSubmit, _children) => {
    ...component,
    initialState: () => {body: ""},
+   reducer: (action, state) =>
+     switch (action) {
+     | SetBody(payload) => ReasonReact.Update({body: payload})
+     | Submit =>
+       ReasonReact.UpdateWithSideEffects(
+         {body: ""},
+         (
+           _self => {
+             let payload = Agent.Comments.create(slug, {"body": state.body});
+             onSubmit(payload);
+           }
+         ),
+       )
+     },
  };

```
A couple of helper methods are needed so let's move our component down and add `setBody` and `createComment` functions at the top of our `make`. These both have the signature `(evt, self)` - the `self` is provided by the `self.handle()` we'll be using later.

Each of these functions utilize a couple of APIs - `ReactEvent.Form.*` for working with the events coming from our form elements. And `self.send()` to dispatch actions within this reducer component.

But why are we using `self.ReasonReact.send()`? Due to some indirection with these functions, we need to tell the type system where the `send()` method is coming from. This is a pattern you'll encounter a lot in ReasonReact.

📄 src/components/Article/CommentInput.re
```diff
  let component = ReasonReact.reducerComponent(__MODULE__);
  
  let make = (~slug, ~onSubmit, _children) => {
-   ...component,
-   initialState: () => {body: ""},
-   reducer: (action, state) =>
-     switch (action) {
-     | SetBody(payload) => ReasonReact.Update({body: payload})
-     | Submit =>
-       ReasonReact.UpdateWithSideEffects(
-         {body: ""},
-         (
-           _self => {
-             let payload = Agent.Comments.create(slug, {"body": state.body});
-             onSubmit(payload);
-           }
-         ),
-       )
-     },
+   let setBody = (evt, self) => {
+     let body = ReactEvent.Form.target(evt)##value;
+     self.ReasonReact.send(SetBody(body));
+   };
+ 
+   let createComment = (evt, self) => {
+     ReactEvent.Form.preventDefault(evt);
+     self.ReasonReact.send(Submit);
+   };
+ 
+   {
+     ...component,
+     initialState: () => {body: ""},
+     reducer: (action, state) =>
+       switch (action) {
+       | SetBody(payload) => ReasonReact.Update({body: payload})
+       | Submit =>
+         ReasonReact.UpdateWithSideEffects(
+           {body: ""},
+           (
+             _self => {
+               let payload =
+                 Agent.Comments.create(slug, {"body": state.body});
+               onSubmit(payload);
+             }
+           ),
+         )
+       },
+   };
  };

```
All the plumbing of this component has been completed, so we can drop in the `render` function and wire up `self.state.body`, `self.handle(createComment)`, and `self.handle(setBody)`.

📄 src/components/Article/CommentInput.re
```diff
  
  let component = ReasonReact.reducerComponent(__MODULE__);
  
- let make = (~slug, ~onSubmit, _children) => {
+ let make = (~slug, ~onSubmit, ~currentUser, _children) => {
    let setBody = (evt, self) => {
      let body = ReactEvent.Form.target(evt)##value;
      self.ReasonReact.send(SetBody(body));
            ),
          )
        },
+     render: self =>
+       <form
+         className="card comment-form" onSubmit={self.handle(createComment)}>
+         <div className="card-block">
+           <textarea
+             className="form-control"
+             placeholder="Write a comment..."
+             value={self.state.body}
+             onChange={self.handle(setBody)}
+             rows=3
+           />
+         </div>
+         <div className="card-footer">
+           <img
+             src=currentUser##image
+             className="comment-author-img"
+             alt=currentUser##username
+           />
+           <button className="btn btn-sm btn-primary" type_="submit">
+             {ReasonReact.string("Post Comment")}
+           </button>
+         </div>
+       </form>,
    };
  };

```
### CommentList cleanup

Since we'll be using `CommentList` exclusively in Reason, we can clean up some of our earlier bindings and get better results from the type system. This also showed us that our quick-and-dirty bindings were missing quite a lot!

And our type system caught a bug! Our `comment##id` is an `int` but the type system inferred a string because we were combining it with another string.

📄 src/agent.re
```diff
      requestPost("/articles/" ++ slug ++ "/comments", {"comment": comment});
  
    let delete = (slug, commentId) =>
-     requestDel("/articles/" ++ slug ++ "/comments/" ++ commentId);
+     requestDel(
+       "/articles/" ++ slug ++ "/comments/" ++ string_of_int(commentId),
+     );
  };

```
📄 src/components/Article/CommentList.re
```diff
- module Comment = {
-   [@bs.module "./Comment.js"]
-   external reactClass: ReasonReact.reactClass = "default";
- 
-   let make = (~comment, ~currentUser, ~slug, _children) =>
-     ReasonReact.wrapJsForReason(
-       ~reactClass,
-       ~props={"comment": comment, "currentUser": currentUser, "slug": slug},
-       [||],
-     );
- };
- 
  let component = ReasonReact.statelessComponent(__MODULE__);
  
- let make = (~comments, ~slug, ~currentUser, _children) => {
+ let make = (~comments, ~slug, ~currentUser=?, ~onDelete, _children) => {
    ...component,
    render: _self =>
      <div>
        ...{
             Belt.Array.map(comments, comment =>
-              <Comment comment currentUser slug key=comment##id />
+              <Comment
+                comment
+                ?currentUser
+                slug
+                onDelete
+                key={string_of_int(comment##id)}
+              />
             )
           }
      </div>,
  };
- 
- let default =
-   ReasonReact.wrapReasonForJs(~component, props =>
-     make(
-       ~comments=props##comments,
-       ~slug=props##slug,
-       ~currentUser=props##currentUser,
-       [||],
-     )
-   );

```
### CommentContainer

Now that the dependencies of CommentContainer are all converted to Reason, we have a really easy time converting it too. One difference is that we change the `currentUser` prop to optional and then convert the if-statement to a switch that unwraps the optional value.

We'll also thread `onDelete` through the component.

📄 src/components/Article/CommentContainer.re
```re
let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~currentUser=?, ~slug, ~comments, ~onSubmit, ~onDelete, _children) => {
  ...component,
  render: _self =>
    switch (currentUser) {
    | Some(currentUser) =>
      <div className="col-xs-12 col-md-8 offset-md-2">
        <div> <CommentInput slug currentUser onSubmit /> </div>
        <CommentList comments slug currentUser onDelete />
      </div>
    | None =>
      <div className="col-xs-12 col-md-8 offset-md-2">
        <p>
          <Link _to="/login"> {ReasonReact.string("Sign in")} </Link>
          {ReasonReact.string(" or ")}
          <Link _to="/register"> {ReasonReact.string("sign up")} </Link>
          {ReasonReact.string(" to add comments on this article.")}
        </p>
        <CommentList comments slug ?currentUser onDelete />
      </div>
    },
};

```
### ListErrors (bonus bug fix!)

During the conversion we found a bug in the original code! Our CommentContainer component was using a `list-errors` element and we have no warning or indication this was an issue. Now that we found it, we can convert our ListErrors component to Reason and then use it in our CommentContainer.

A bonus is that BuckleScript has a `Js.Dict.entries()` API so we don't have to use the clunky `Object.keys()` technique like the original.

And even during the conversion of the ListErrors component, we found a visual bug that was caused by rendering an array of strings as a React element. Having a type system over this whole application would have caught that issue before it even happened. We've fixed the Reason version of ListErrors so it knows the shape of the errors prop.

📄 src/components/Article/CommentContainer.re
```diff
  let component = ReasonReact.statelessComponent(__MODULE__);
  
- let make = (~currentUser=?, ~slug, ~comments, ~onSubmit, ~onDelete, _children) => {
+ let make =
+     (
+       ~currentUser=?,
+       ~slug,
+       ~comments,
+       ~onSubmit,
+       ~onDelete,
+       ~errors=?,
+       _children,
+     ) => {
    ...component,
    render: _self =>
      switch (currentUser) {
      | Some(currentUser) =>
        <div className="col-xs-12 col-md-8 offset-md-2">
-         <div> <CommentInput slug currentUser onSubmit /> </div>
+         <div>
+           <ListErrors ?errors />
+           <CommentInput slug currentUser onSubmit />
+         </div>
          <CommentList comments slug currentUser onDelete />
        </div>
      | None =>

```
📄 src/components/ListErrors.re
```re
let component = ReasonReact.statelessComponent(__MODULE__);

let make = (~errors=?, _children) => {
  ...component,
  render: _self =>
    switch (errors) {
    | Some(errors) =>
      <ul className="error-messages">
        ...{
             Js.Dict.entries(errors)
             ->Belt.Array.map(((key, messages)) =>
                 <li key>
                   {ReasonReact.string(key ++ ": ")}
                   {Js.Array.joinWith(", ", messages)->ReasonReact.string}
                 </li>
               )
           }
      </ul>
    | None => ReasonReact.null
    },
};

```
We can add our `default` export to `ListErrors.re` and then remove the old `ListErrors.js` component. This component can now be used seamlessly by JS and Reason.

📄 src/components/Editor.js
```diff
- import ListErrors from './ListErrors';
+ import ListErrors from './ListErrors.bs.js';
  import React from 'react';
  import agent from '../agent';
  import { connect } from 'react-redux';

```
📄 src/components/ListErrors.js
```diff
- import React from 'react';
- 
- class ListErrors extends React.Component {
-   render() {
-     const errors = this.props.errors;
-     if (errors) {
-       return (
-         <ul className="error-messages">
-           {
-             Object.keys(errors).map(key => {
-               return (
-                 <li key={key}>
-                   {key} {errors[key]}
-                 </li>
-               );
-             })
-           }
-         </ul>
-       );
-     } else {
-       return null;
-     }
-   }
- }
- 
- export default ListErrors;

```
📄 src/components/ListErrors.re
```diff
      | None => ReasonReact.null
      },
  };
+ 
+ let default =
+   ReasonReact.wrapReasonForJs(~component, props =>
+     make(~errors=?Js.Nullable.toOption(props##errors), [||])
+   );

```
📄 src/components/Login.js
```diff
  import { Link } from 'react-router-dom';
- import ListErrors from './ListErrors';
+ import ListErrors from './ListErrors.bs.js';
  import React from 'react';
  import agent from '../agent';
  import { connect } from 'react-redux';

```
📄 src/components/Register.js
```diff
  import { Link } from 'react-router-dom';
- import ListErrors from './ListErrors';
+ import ListErrors from './ListErrors.bs.js';
  import React from 'react';
  import agent from '../agent';
  import { connect } from 'react-redux';

```
📄 src/components/Settings.js
```diff
- import ListErrors from './ListErrors';
+ import ListErrors from './ListErrors.bs.js';
  import React from 'react';
  import agent from '../agent';
  import { connect } from 'react-redux';

```
### Article

Up until this point, we've been playing very fast-and-loose with our types, especially on objects coming from JS. Since we are now converting the `Article` component, we should get (at least a little) more serious about our types. These still aren't records, but at least we are defining the shape we are expecting to come back from our endpoints. Note: the better way to do this would be to decode a `Js.Json.t` type into concrete records but that exceeds our scope.

📄 src/components/Article/Article.re
```re
type author = {
  .
  "bio": Js.Nullable.t(string),
  "following": bool,
  "image": string,
  "username": string,
};

type comment = {
  .
  "author": author,
  "body": string,
  "createdAt": string,
  "id": int,
  "updatedAt": string,
};

type article = {
  .
  "author": author,
  "body": string,
  "createdAt": string,
  "description": string,
  "favorited": bool,
  "favoritesCount": int,
  "slug": string,
  "tagList": array(string),
  "title": string,
  "updatedAt": string,
};

type errors = Js.Dict.t(array(string));

```
Next, we'll set up our `state` and `action` types for our reducer component, like we did previously in the CommentInput component. These were taken from the `article` reducer but `ADD_COMMENT` was split into an `AddComment` and `CommentError` action because the JS behavior was buggy.

📄 src/components/Article/Article.re
```diff
  };
  
  type errors = Js.Dict.t(array(string));
+ 
+ type state = {
+   article: option(article),
+   comments: array(comment),
+   errors: option(errors),
+ };
+ 
+ type action =
+   | Loaded(article, array(comment))
+   | CommentError(errors)
+   | AddComment(comment)
+   | DeleteComment(int);
+ 
+ let component = ReasonReact.reducerComponent(__MODULE__);

```
Our `initialState` will use `None` for `article` and `errors` because we don't have any. An empty array was used for `comments` to concat onto later.

📄 src/components/Article/Article.re
```diff
    | DeleteComment(int);
  
  let component = ReasonReact.reducerComponent(__MODULE__);
+ 
+ let make = _children => {
+   ...component,
+   initialState: () => {article: None, comments: [||], errors: None},
+ };

```
And we'll set up all our reducer logic - this was also ripped straight from the redux store.

📄 src/components/Article/Article.re
```diff
  let make = _children => {
    ...component,
    initialState: () => {article: None, comments: [||], errors: None},
+   reducer: (action, state) =>
+     switch (action) {
+     | Loaded(article, comments) =>
+       ReasonReact.Update({...state, article: Some(article), comments})
+     | CommentError(errors) =>
+       ReasonReact.Update({...state, errors: Some(errors)})
+     | AddComment(comment) =>
+       ReasonReact.Update({
+         ...state,
+         errors: None,
+         comments: Belt.Array.concat(state.comments, [|comment|]),
+       })
+     | DeleteComment(commentId) =>
+       ReasonReact.Update({
+         ...state,
+         errors: None,
+         comments:
+           Belt.Array.keep(state.comments, comment =>
+             comment##id !== commentId
+           ),
+       })
+     },
  };

```
This component utilizes a `componentWillMount` hook in JS but ReasonReact doesn't support that lifecycle method so we use `didMount`.

We also had to use `Js.Promise.all2` because the 2 methods return different types of data - and every promise in `Js.Promise.all` must have matching types.

As mentioned previously, we're playing fast-and-loose with types so we had to relax the return value of our `agent.re` to a generic instead of `Js.Json.t`.

📄 src/agent.re
```diff
  [@bs.send] external post: (client, string, 'body) => client = "";
  [@bs.send] external use: (client, 'request => unit) => client = "";
  [@bs.send]
- external then_: (client, 'response => Js.Json.t) => Js.Promise.t(Js.Json.t) =
+ external then_: (client, 'response => Js.Json.t) => Js.Promise.t('result) =
    "then";
  
  let requestGet = url =>
    ->then_(responseBody);
  
  module Articles = {
+   let get = slug => requestGet("/articles/" ++ slug);
    let del = slug => requestDel("/articles/" ++ slug);
  };
  
  module Comments = {
    let create = (slug, comment) =>
      requestPost("/articles/" ++ slug ++ "/comments", {"comment": comment});
- 
    let delete = (slug, commentId) =>
      requestDel(
        "/articles/" ++ slug ++ "/comments/" ++ string_of_int(commentId),
      );
+   let forArticle = slug => requestGet("/articles/" ++ slug ++ "/comments");
  };

```
📄 src/components/Article/Article.re
```diff
  
  let component = ReasonReact.reducerComponent(__MODULE__);
  
- let make = _children => {
+ let make = (~match, _children) => {
    ...component,
    initialState: () => {article: None, comments: [||], errors: None},
    reducer: (action, state) =>
            ),
        })
      },
+   didMount: self =>
+     Js.Promise.all2((
+       Agent.Articles.get(match##params##id),
+       Agent.Comments.forArticle(match##params##id),
+     ))
+     |> Js.Promise.then_(((articleResult, commentsResult)) => {
+          self.send(Loaded(articleResult##article, commentsResult##comments));
+          Js.Promise.resolve();
+        })
+     |> ignore,
  };

```
We've been threading methods throughout many of our components, and we're finally at the point where they are defined because they interact with the Article component's reducer.

Here we define `onCommentDelete` and `onCommentCreate` functions that wait for an API response and dispatch results into our reducer.

In Reason, `Js.Promise.catch` is a painful API to work with so we created a (very hacky) external that converts from some type to any other type using BuckleScripts "%identity". __Try your hardest not to use this and be extremely careful if you must.__ We inspected the shape of the object our `agent` APIs get rejected with and found that we can look up the response using this hack.

📄 src/components/Article/Article.re
```diff
  
  let component = ReasonReact.reducerComponent(__MODULE__);
  
+ external errorToJsObj: Js.Promise.error => 'jsObj = "%identity";
+ let onCommentDelete = (result, commentId, self) =>
+   result
+   |> Js.Promise.then_(_result => {
+        self.ReasonReact.send(DeleteComment(commentId));
+        Js.Promise.resolve();
+      })
+   |> ignore;
+ 
+ let onCommentCreate = (result, self) =>
+   result
+   |> Js.Promise.then_(result => {
+        self.ReasonReact.send(AddComment(result##comment));
+        Js.Promise.resolve();
+      })
+   |> Js.Promise.catch(error => {
+        let errors = errorToJsObj(error)##response##body##errors;
+        self.ReasonReact.send(CommentError(errors));
+        Js.Promise.resolve();
+      })
+   |> ignore;
+ 
  let make = (~match, _children) => {
    ...component,
    initialState: () => {article: None, comments: [||], errors: None},

```
We'll also need to use an external for the `marked` module.

📄 src/components/Article/Article.re
```diff
  
  let component = ReasonReact.reducerComponent(__MODULE__);
  
+ [@bs.module] external marked: (string, 'options) => string = "marked";
+ 
  external errorToJsObj: Js.Promise.error => 'jsObj = "%identity";
  let onCommentDelete = (result, commentId, self) =>
    result

```
Everything is now prepared for our `render` method so we can drop that in. We're going to thread the `onArticleDelete` prop from JS since it interacts with a different reducer in the redux store.

📄 src/components/Article/Article.re
```diff
       })
    |> ignore;
  
- let make = (~match, _children) => {
+ let make = (~match, ~currentUser=?, ~onArticleDelete, _children) => {
    ...component,
    initialState: () => {article: None, comments: [||], errors: None},
    reducer: (action, state) =>
           Js.Promise.resolve();
         })
      |> ignore,
+   render: self =>
+     switch (self.state.article) {
+     | None => ReasonReact.null
+     | Some(article) =>
+       let canModify =
+         switch (currentUser) {
+         | Some(currentUser) =>
+           currentUser##username === article##author##username
+         | None => false
+         };
+       let markup = {"__html": marked(article##body, {"sanitize": true})};
+ 
+       <div className="article-page">
+         <div className="banner">
+           <div className="container">
+             <h1> {ReasonReact.string(article##title)} </h1>
+             <ArticleMeta article canModify onClickDelete=onArticleDelete />
+           </div>
+         </div>
+         <div className="container page">
+           <div className="row article-content">
+             <div className="col-xs-12">
+               <div dangerouslySetInnerHTML=markup />
+               <ul className="tag-list">
+                 ...{
+                      Belt.Array.map(article##tagList, tag =>
+                        <li
+                          className="tag-default tag-pill tag-outline" key=tag>
+                          {ReasonReact.string(tag)}
+                        </li>
+                      )
+                    }
+               </ul>
+             </div>
+           </div>
+           <hr />
+           <div className="article-actions" />
+           <div className="row">
+             <CommentContainer
+               onSubmit={self.handle(onCommentCreate)}
+               onDelete=(
+                 (result, commentId) =>
+                   onCommentDelete(result, commentId, self)
+               )
+               comments={self.state.comments}
+               errors=?{self.state.errors}
+               slug=match##params##id
+               ?currentUser
+             />
+           </div>
+         </div>
+       </div>;
+     },
  };

```
Once more, we wire up a `default` export that maps our Reason component to JS. The interesting thing here is that we must use `[%bs.raw {| props.match |}]` because Reason mangles the name `match` when used like `props##match` (TIL).

📄 src/components/Article/Article.re
```diff
        </div>;
      },
  };
+ 
+ let default =
+   ReasonReact.wrapReasonForJs(~component, props =>
+     make(
+       ~currentUser=?Js.Nullable.toOption(props##currentUser),
+       ~match=[%bs.raw {| props.match |}],
+       ~onArticleDelete=props##onArticleDelete,
+       [||],
+     )
+   );

```
Let's swap our Article component in and wire up the last `onArticleDelete` prop.

📄 src/components/App.js
```diff
  import Header from './Header';
  import React from 'react';
  import { connect } from 'react-redux';
- import { APP_LOAD, REDIRECT } from '../constants/actionTypes';
+ import { APP_LOAD, REDIRECT, DELETE_ARTICLE } from '../constants/actionTypes';
  import { Route, Switch } from 'react-router-dom';
- import Article from '../components/Article';
+ import Article from '../components/Article/Article.bs.js';
  import Editor from '../components/Editor';
  import Home from '../components/Home';
  import Login from '../components/Login';
    onLoad: (payload, token) =>
      dispatch({ type: APP_LOAD, payload, token, skipTracking: true }),
    onRedirect: () =>
-     dispatch({ type: REDIRECT })
+     dispatch({ type: REDIRECT }),
+   onArticleDelete: (payload) =>
+     dispatch({ type: DELETE_ARTICLE, payload })
  });
  
  class App extends React.Component {
              <Route path="/register" component={Register} />
              <Route path="/editor/:slug" component={Editor} />
              <Route path="/editor" component={Editor} />
-             <Route path="/article/:id" render={(props) => <Article {...props} currentUser={this.props.currentUser} />} />
+             <Route path="/article/:id" render={(props) => <Article {...props} currentUser={this.props.currentUser} onArticleDelete={this.props.onArticleDelete} />} />
              <Route path="/settings" component={Settings} />
              <Route path="/@:username/favorites" component={ProfileFavorites} />
              <Route path="/@:username" component={Profile} />

```
## Delete the JS!

And now we get to delete a massive chunk of JS (and global state)!!

📄 src/components/Article/ArticleActions.js
```diff
- import { Link } from 'react-router-dom';
- import React from 'react';
- import agent from '../../agent';
- import { connect } from 'react-redux';
- import { DELETE_ARTICLE } from '../../constants/actionTypes';
- 
- const mapDispatchToProps = dispatch => ({
-   onClickDelete: payload =>
-     dispatch({ type: DELETE_ARTICLE, payload })
- });
- 
- const ArticleActions = props => {
-   const article = props.article;
-   const del = () => {
-     props.onClickDelete(agent.Articles.del(article.slug))
-   };
-   if (props.canModify) {
-     return (
-       <span>
- 
-         <Link
-           to={`/editor/${article.slug}`}
-           className="btn btn-outline-secondary btn-sm">
-           <i className="ion-edit"></i> Edit Article
-         </Link>
- 
-         <button className="btn btn-outline-danger btn-sm" onClick={del}>
-           <i className="ion-trash-a"></i> Delete Article
-         </button>
- 
-       </span>
-     );
-   }
- 
-   return (
-     <span>
-     </span>
-   );
- };
- 
- export default connect(() => ({}), mapDispatchToProps)(ArticleActions);

```
📄 src/components/Article/ArticleMeta.js
```diff
- import ArticleActions from './ArticleActions';
- import { Link } from 'react-router-dom';
- import React from 'react';
- 
- const ArticleMeta = props => {
-   const article = props.article;
-   return (
-     <div className="article-meta">
-       <Link to={`/@${article.author.username}`}>
-         <img src={article.author.image} alt={article.author.username} />
-       </Link>
- 
-       <div className="info">
-         <Link to={`/@${article.author.username}`} className="author">
-           {article.author.username}
-         </Link>
-         <span className="date">
-           {new Date(article.createdAt).toDateString()}
-         </span>
-       </div>
- 
-       <ArticleActions canModify={props.canModify} article={article} />
-     </div>
-   );
- };
- 
- export default ArticleMeta;

```
📄 src/components/Article/Comment.js
```diff
- import DeleteButton from './DeleteButton';
- import { Link } from 'react-router-dom';
- import React from 'react';
- 
- const Comment = props => {
-   const comment = props.comment;
-   const show = props.currentUser &&
-     props.currentUser.username === comment.author.username;
-   return (
-     <div className="card">
-       <div className="card-block">
-         <p className="card-text">{comment.body}</p>
-       </div>
-       <div className="card-footer">
-         <Link
-           to={`/@${comment.author.username}`}
-           className="comment-author">
-           <img src={comment.author.image} className="comment-author-img" alt={comment.author.username} />
-         </Link>
-         &nbsp;
-         <Link
-           to={`/@${comment.author.username}`}
-           className="comment-author">
-           {comment.author.username}
-         </Link>
-         <span className="date-posted">
-           {new Date(comment.createdAt).toDateString()}
-         </span>
-         <DeleteButton show={show} slug={props.slug} commentId={comment.id} />
-       </div>
-     </div>
-   );
- };
- 
- export default Comment;

```
📄 src/components/Article/CommentContainer.js
```diff
- import CommentInput from './CommentInput';
- import CommentList from './CommentList.bs.js';
- import { Link } from 'react-router-dom';
- import React from 'react';
- 
- const CommentContainer = props => {
-   if (props.currentUser) {
-     return (
-       <div className="col-xs-12 col-md-8 offset-md-2">
-         <div>
-           <list-errors errors={props.errors}></list-errors>
-           <CommentInput slug={props.slug} currentUser={props.currentUser} />
-         </div>
- 
-         <CommentList
-           comments={props.comments}
-           slug={props.slug}
-           currentUser={props.currentUser} />
-       </div>
-     );
-   } else {
-     return (
-       <div className="col-xs-12 col-md-8 offset-md-2">
-         <p>
-           <Link to="/login">Sign in</Link>
-           &nbsp;or&nbsp;
-           <Link to="/register">sign up</Link>
-           &nbsp;to add comments on this article.
-         </p>
- 
-         <CommentList
-           comments={props.comments}
-           slug={props.slug}
-           currentUser={props.currentUser} />
-       </div>
-     );
-   }
- };
- 
- export default CommentContainer;

```
📄 src/components/Article/CommentInput.js
```diff
- import React from 'react';
- import agent from '../../agent';
- import { connect } from 'react-redux';
- import { ADD_COMMENT } from '../../constants/actionTypes';
- 
- const mapDispatchToProps = dispatch => ({
-   onSubmit: payload =>
-     dispatch({ type: ADD_COMMENT, payload })
- });
- 
- class CommentInput extends React.Component {
-   constructor() {
-     super();
-     this.state = {
-       body: ''
-     };
- 
-     this.setBody = ev => {
-       this.setState({ body: ev.target.value });
-     };
- 
-     this.createComment = ev => {
-       ev.preventDefault();
-       const payload = agent.Comments.create(this.props.slug,
-         { body: this.state.body });
-       this.setState({ body: '' });
-       this.props.onSubmit(payload);
-     };
-   }
- 
-   render() {
-     return (
-       <form className="card comment-form" onSubmit={this.createComment}>
-         <div className="card-block">
-           <textarea className="form-control"
-             placeholder="Write a comment..."
-             value={this.state.body}
-             onChange={this.setBody}
-             rows="3">
-           </textarea>
-         </div>
-         <div className="card-footer">
-           <img
-             src={this.props.currentUser.image}
-             className="comment-author-img"
-             alt={this.props.currentUser.username} />
-           <button
-             className="btn btn-sm btn-primary"
-             type="submit">
-             Post Comment
-           </button>
-         </div>
-       </form>
-     );
-   }
- }
- 
- export default connect(() => ({}), mapDispatchToProps)(CommentInput);

```
📄 src/components/Article/DeleteButton.js
```diff
- import React from 'react';
- import agent from '../../agent';
- import { connect } from 'react-redux';
- import { DELETE_COMMENT } from '../../constants/actionTypes';
- 
- const mapDispatchToProps = dispatch => ({
-   onClick: (payload, commentId) =>
-     dispatch({ type: DELETE_COMMENT, payload, commentId })
- });
- 
- const DeleteButton = props => {
-   const del = () => {
-     const payload = agent.Comments.delete(props.slug, props.commentId);
-     props.onClick(payload, props.commentId);
-   };
- 
-   if (props.show) {
-     return (
-       <span className="mod-options">
-         <i className="ion-trash-a" onClick={del}></i>
-       </span>
-     );
-   }
-   return null;
- };
- 
- export default connect(() => ({}), mapDispatchToProps)(DeleteButton);

```
📄 src/components/Article/index.js
```diff
- import ArticleMeta from './ArticleMeta';
- import CommentContainer from './CommentContainer';
- import React from 'react';
- import agent from '../../agent';
- import { connect } from 'react-redux';
- import marked from 'marked';
- import { ARTICLE_PAGE_LOADED, ARTICLE_PAGE_UNLOADED } from '../../constants/actionTypes';
- 
- const mapStateToProps = state => ({
-   ...state.article,
- });
- 
- const mapDispatchToProps = dispatch => ({
-   onLoad: payload =>
-     dispatch({ type: ARTICLE_PAGE_LOADED, payload }),
-   onUnload: () =>
-     dispatch({ type: ARTICLE_PAGE_UNLOADED })
- });
- 
- class Article extends React.Component {
-   componentWillMount() {
-     this.props.onLoad(Promise.all([
-       agent.Articles.get(this.props.match.params.id),
-       agent.Comments.forArticle(this.props.match.params.id)
-     ]));
-   }
- 
-   componentWillUnmount() {
-     this.props.onUnload();
-   }
- 
-   render() {
-     if (!this.props.article) {
-       return null;
-     }
- 
-     const markup = { __html: marked(this.props.article.body, { sanitize: true }) };
-     const canModify = this.props.currentUser &&
-       this.props.currentUser.username === this.props.article.author.username;
-     return (
-       <div className="article-page">
- 
-         <div className="banner">
-           <div className="container">
- 
-             <h1>{this.props.article.title}</h1>
-             <ArticleMeta
-               article={this.props.article}
-               canModify={canModify} />
- 
-           </div>
-         </div>
- 
-         <div className="container page">
- 
-           <div className="row article-content">
-             <div className="col-xs-12">
- 
-               <div dangerouslySetInnerHTML={markup}></div>
- 
-               <ul className="tag-list">
-                 {
-                   this.props.article.tagList.map(tag => {
-                     return (
-                       <li
-                         className="tag-default tag-pill tag-outline"
-                         key={tag}>
-                         {tag}
-                       </li>
-                     );
-                   })
-                 }
-               </ul>
- 
-             </div>
-           </div>
- 
-           <hr />
- 
-           <div className="article-actions">
-           </div>
- 
-           <div className="row">
-             <CommentContainer
-               comments={this.props.comments || []}
-               errors={this.props.commentErrors}
-               slug={this.props.match.params.id}
-               currentUser={this.props.currentUser} />
-           </div>
-         </div>
-       </div>
-     );
-   }
- }
- 
- export default connect(mapStateToProps, mapDispatchToProps)(Article);

```
📄 src/constants/actionTypes.js
```diff
  export const SETTINGS_PAGE_UNLOADED = 'SETTINGS_PAGE_UNLOADED';
  export const HOME_PAGE_LOADED = 'HOME_PAGE_LOADED';
  export const HOME_PAGE_UNLOADED = 'HOME_PAGE_UNLOADED';
- export const ARTICLE_PAGE_LOADED = 'ARTICLE_PAGE_LOADED';
- export const ARTICLE_PAGE_UNLOADED = 'ARTICLE_PAGE_UNLOADED';
- export const ADD_COMMENT = 'ADD_COMMENT';
- export const DELETE_COMMENT = 'DELETE_COMMENT';
  export const ARTICLE_FAVORITED = 'ARTICLE_FAVORITED';
  export const ARTICLE_UNFAVORITED = 'ARTICLE_UNFAVORITED';
  export const SET_PAGE = 'SET_PAGE';

```
📄 src/reducer.js
```diff
- import article from './reducers/article';
  import articleList from './reducers/articleList';
  import auth from './reducers/auth';
  import { combineReducers } from 'redux';
  import { routerReducer } from 'react-router-redux';
  
  export default combineReducers({
-   article,
    articleList,
    auth,
    common,

```
📄 src/reducers/article.js
```diff
- import {
-   ARTICLE_PAGE_LOADED,
-   ARTICLE_PAGE_UNLOADED,
-   ADD_COMMENT,
-   DELETE_COMMENT
- } from '../constants/actionTypes';
- 
- export default (state = {}, action) => {
-   switch (action.type) {
-     case ARTICLE_PAGE_LOADED:
-       return {
-         ...state,
-         article: action.payload[0].article,
-         comments: action.payload[1].comments
-       };
-     case ARTICLE_PAGE_UNLOADED:
-       return {};
-     case ADD_COMMENT:
-       return {
-         ...state,
-         commentErrors: action.error ? action.payload.errors : null,
-         comments: action.error ?
-           null :
-           (state.comments || []).concat([action.payload.comment])
-       };
-     case DELETE_COMMENT:
-       const commentId = action.commentId
-       return {
-         ...state,
-         comments: state.comments.filter(comment => comment.id !== commentId)
-       };
-     default:
-       return state;
-   }
- };

```
📄 src/reducers/common.js
```diff
    LOGIN,
    REGISTER,
    DELETE_ARTICLE,
-   ARTICLE_PAGE_UNLOADED,
    EDITOR_PAGE_UNLOADED,
    HOME_PAGE_UNLOADED,
    PROFILE_PAGE_UNLOADED,
        };
      case DELETE_ARTICLE:
        return { ...state, redirectTo: '/' };
-     case ARTICLE_PAGE_UNLOADED:
      case EDITOR_PAGE_UNLOADED:
      case HOME_PAGE_UNLOADED:
      case PROFILE_PAGE_UNLOADED:

```