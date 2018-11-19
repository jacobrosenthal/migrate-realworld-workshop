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

let make = _children => {
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
};
