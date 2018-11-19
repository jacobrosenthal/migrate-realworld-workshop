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

type state = {
  article: option(article),
  comments: array(comment),
  errors: option(errors),
};

type action =
  | Loaded(article, array(comment))
  | CommentError(errors)
  | AddComment(comment)
  | DeleteComment(int);

let component = ReasonReact.reducerComponent(__MODULE__);

[@bs.module] external marked: (string, 'options) => string = "marked";

external errorToJsObj: Js.Promise.error => 'jsObj = "%identity";
let onCommentDelete = (result, commentId, self) =>
  result
  |> Js.Promise.then_(_result => {
       self.ReasonReact.send(DeleteComment(commentId));
       Js.Promise.resolve();
     })
  |> ignore;

let onCommentCreate = (result, self) =>
  result
  |> Js.Promise.then_(result => {
       self.ReasonReact.send(AddComment(result##comment));
       Js.Promise.resolve();
     })
  |> Js.Promise.catch(error => {
       let errors = errorToJsObj(error)##response##body##errors;
       self.ReasonReact.send(CommentError(errors));
       Js.Promise.resolve();
     })
  |> ignore;

let make = (~match, ~currentUser=?, ~onArticleDelete, _children) => {
  ...component,
  initialState: () => {article: None, comments: [||], errors: None},
  reducer: (action, state) =>
    switch (action) {
    | Loaded(article, comments) =>
      ReasonReact.Update({...state, article: Some(article), comments})
    | CommentError(errors) =>
      ReasonReact.Update({...state, errors: Some(errors)})
    | AddComment(comment) =>
      ReasonReact.Update({
        ...state,
        errors: None,
        comments: Belt.Array.concat(state.comments, [|comment|]),
      })
    | DeleteComment(commentId) =>
      ReasonReact.Update({
        ...state,
        errors: None,
        comments:
          Belt.Array.keep(state.comments, comment =>
            comment##id !== commentId
          ),
      })
    },
  didMount: self =>
    Js.Promise.all2((
      Agent.Articles.get(match##params##id),
      Agent.Comments.forArticle(match##params##id),
    ))
    |> Js.Promise.then_(((articleResult, commentsResult)) => {
         self.send(Loaded(articleResult##article, commentsResult##comments));
         Js.Promise.resolve();
       })
    |> ignore,
  render: self =>
    switch (self.state.article) {
    | None => ReasonReact.null
    | Some(article) =>
      let canModify =
        switch (currentUser) {
        | Some(currentUser) =>
          currentUser##username === article##author##username
        | None => false
        };
      let markup = {"__html": marked(article##body, {"sanitize": true})};

      <div className="article-page">
        <div className="banner">
          <div className="container">
            <h1> {ReasonReact.string(article##title)} </h1>
            <ArticleMeta article canModify onClickDelete=onArticleDelete />
          </div>
        </div>
        <div className="container page">
          <div className="row article-content">
            <div className="col-xs-12">
              <div dangerouslySetInnerHTML=markup />
              <ul className="tag-list">
                ...{
                     Belt.Array.map(article##tagList, tag =>
                       <li
                         className="tag-default tag-pill tag-outline" key=tag>
                         {ReasonReact.string(tag)}
                       </li>
                     )
                   }
              </ul>
            </div>
          </div>
          <hr />
          <div className="article-actions" />
          <div className="row">
            <CommentContainer
              onSubmit={self.handle(onCommentCreate)}
              onDelete=(
                (result, commentId) =>
                  onCommentDelete(result, commentId, self)
              )
              comments={self.state.comments}
              errors=?{self.state.errors}
              slug=match##params##id
              ?currentUser
            />
          </div>
        </div>
      </div>;
    },
};

let default =
  ReasonReact.wrapReasonForJs(~component, props =>
    make(
      ~currentUser=?Js.Nullable.toOption(props##currentUser),
      ~match=[%bs.raw {| props.match |}],
      ~onArticleDelete=props##onArticleDelete,
      [||],
    )
  );
