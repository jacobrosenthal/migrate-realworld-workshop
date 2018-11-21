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


let default =
  ReasonReact.wrapReasonForJs(~component, props =>
    make(~errors=?Js.Nullable.toOption(props##errors), [||])
  );
