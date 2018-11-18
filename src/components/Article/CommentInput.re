type state = {body: string};

type action =
  | SetBody(string)
  | Submit;

let component = ReasonReact.reducerComponent(__MODULE__);

let make = (_children) => {
  ...component,
  initialState: () => {body: ""},
};
