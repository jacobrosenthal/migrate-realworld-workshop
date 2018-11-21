type superagent;
type client;
type globalPromise;

[@bs.scope "global"] [@bs.val] external promise: globalPromise = "Promise";

[@bs.module]
external superagentPromise: (superagent, globalPromise) => client = "superagent-promise";

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
