type state = {body: string};

type action =
  | SetBody(string)
  | Submit;

let component = ReasonReact.reducerComponent(__MODULE__);

let make = (~slug, ~onSubmit, _children) => {
  ...component,
  initialState: () => {body: ""},
  reducer: (action, state) =>
    switch (action) {
    | SetBody(payload) => ReasonReact.Update({body: payload})
    | Submit =>
      ReasonReact.UpdateWithSideEffects(
        {body: ""},
        (
          _self => {
            let payload = Agent.Comments.create(slug, {"body": state.body});
            onSubmit(payload);
          }
        ),
      )
    },
};
