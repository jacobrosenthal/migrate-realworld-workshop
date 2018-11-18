type state = {body: string};

type action =
  | SetBody(string)
  | Submit;

let component = ReasonReact.reducerComponent(__MODULE__);

let make = (~slug, ~onSubmit, ~currentUser, _children) => {
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
    render: self =>
      <form
        className="card comment-form" onSubmit={self.handle(createComment)}>
        <div className="card-block">
          <textarea
            className="form-control"
            placeholder="Write a comment..."
            value={self.state.body}
            onChange={self.handle(setBody)}
            rows=3
          />
        </div>
        <div className="card-footer">
          <img
            src=currentUser##image
            className="comment-author-img"
            alt=currentUser##username
          />
          <button className="btn btn-sm btn-primary" type_="submit">
            {ReasonReact.string("Post Comment")}
          </button>
        </div>
      </form>,
  };
};
