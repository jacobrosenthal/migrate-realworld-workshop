type state = {body: string};

type action =
  | SetBody(string)
  | Submit;

let component = ReasonReact.reducerComponent(__MODULE__);

let make = (~slug, ~onSubmit, _children) => {
  let setBody = (evt, self) => {
    let body = ReactEvent.Form.target(evt)##value;
    self.ReasonReact.send(SetBody(body));
  };

  let createComment = (evt, self) => {
    ReactEvent.Form.preventDefault(evt);
    self.ReasonReact.send(Submit);
  };

  {
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
              let payload =
                Agent.Comments.create(slug, {"body": state.body});
              onSubmit(payload);
            }
          ),
        )
      },
  };
};
