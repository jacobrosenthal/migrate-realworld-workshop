[@bs.module "react-router-dom"]
external reactClass: ReasonReact.reactClass = "Link";

let make = (~_to, ~className=?, children) =>
  ReasonReact.wrapJsForReason(
    ~reactClass,
    ~props={"to": _to, "className": Js.Nullable.fromOption(className)},
    children,
  );
