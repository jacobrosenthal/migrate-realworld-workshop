type superagent;
type client;
type globalPromise;

[@bs.scope "global"] [@bs.val] external promise: globalPromise = "Promise";

[@bs.module]
external superagentPromise: (superagent, globalPromise) => client =
  "superagent-promise";
[@bs.module] external superagent_: superagent = "superagent";

let superagent = superagentPromise(superagent_, promise);

let responseBody = res => res##body;
