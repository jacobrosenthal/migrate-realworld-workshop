type superagent;
type client;
type globalPromise;

[@bs.scope "global"] [@bs.val] external promise: globalPromise = "Promise";

[@bs.module]
external superagentPromise: (superagent, globalPromise) => client =
  "superagent-promise";
[@bs.module] external superagent_: superagent = "superagent";

let superagent = superagentPromise(superagent_, promise);

let apiRoot = "https://conduit.productionready.io/api";

let responseBody = res => res##body;

let token = ref(None);
let tokenPlugin = req =>
  switch (token^) {
  | Some(token) => req##set("authorization", "Token " ++ token)
  | None => ()
  };

let setToken = _token => token := Some(_token);

[@bs.send] external get: (client, string) => client = "";
[@bs.send] external del: (client, string) => client = "";
[@bs.send] external put: (client, string, 'body) => client = "";
[@bs.send] external post: (client, string, 'body) => client = "";
[@bs.send] external use: (client, 'request => unit) => client = "";
[@bs.send]
external then_: (client, 'response => Js.Json.t) => Js.Promise.t(Js.Json.t) =
  "then";

let requestGet = url =>
  superagent->get(apiRoot ++ url)->use(tokenPlugin)->then_(responseBody);

let requestDel = url =>
  superagent->del(apiRoot ++ url)->use(tokenPlugin)->then_(responseBody);

let requestPut = (url, body) =>
  superagent
  ->put(apiRoot ++ url, body)
  ->use(tokenPlugin)
  ->then_(responseBody);

let requestPost = (url, body) =>
  superagent
  ->post(apiRoot ++ url, body)
  ->use(tokenPlugin)
  ->then_(responseBody);

module Articles = {
  let del = slug => requestDel("/articles/" ++ slug);
};

module Comments = {
  let create = (slug, comment) =>
    requestPost("/articles/" ++ slug ++ "/comments", {"comment": comment});

  let delete = (slug, commentId) =>
    requestDel("/articles/" ++ slug ++ "/comments/" ++ commentId);
};
